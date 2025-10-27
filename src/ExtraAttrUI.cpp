#include "ExtraAttrUI.h"
#include "MayaUtils.h"
#include <QHeaderView>
#include <QMessageBox>
#include <QInputDialog>
#include <QProgressDialog>
#include <QApplication>
#include <maya/MQtUtil.h>
#include <maya/MGlobal.h>
#include <maya/MSelectionList.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MPlug.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MFnEnumAttribute.h>
#include <maya/MFnNumericData.h>

// ========== EnumAttributeDelegate Implementation ==========

EnumAttributeDelegate::EnumAttributeDelegate(ExtraAttrUI* parent)
    : QStyledItemDelegate(parent)
    , m_ui(parent)
{
}

QWidget* EnumAttributeDelegate::createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    Q_UNUSED(option);

    // Show dropdown only for Value column
    if (index.column() != NodeAttributeModel::COL_VALUE) {
        return QStyledItemDelegate::createEditor(parent, option, index);
    }

    // Get node name and attribute name
    NodeAttributeModel* model = qobject_cast<NodeAttributeModel*>(const_cast<QAbstractItemModel*>(index.model()));
    if (!model) {
        return QStyledItemDelegate::createEditor(parent, option, index);
    }

    QString nodeName = model->getNodeName(index.row());
    QString attrName = model->getCurrentAttributeName();

    // Get enum attribute options
    QStringList enumFields = m_ui->getEnumFieldNames(nodeName, attrName);
    if (enumFields.isEmpty()) {
        // Return normal editor if not an enum attribute
        return QStyledItemDelegate::createEditor(parent, option, index);
    }

    // Create dropdown
    QComboBox* comboBox = new QComboBox(parent);
    comboBox->addItems(enumFields);
    return comboBox;
}

void EnumAttributeDelegate::setEditorData(QWidget* editor, const QModelIndex& index) const
{
    QComboBox* comboBox = qobject_cast<QComboBox*>(editor);
    if (comboBox) {
        QString currentValue = index.model()->data(index, Qt::EditRole).toString();
        int currentIndex = comboBox->findText(currentValue);
        if (currentIndex >= 0) {
            comboBox->setCurrentIndex(currentIndex);
        }
    } else {
        QStyledItemDelegate::setEditorData(editor, index);
    }
}

void EnumAttributeDelegate::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const
{
    QComboBox* comboBox = qobject_cast<QComboBox*>(editor);
    if (comboBox) {
        QString value = comboBox->currentText();
        model->setData(index, value, Qt::EditRole);
    } else {
        QStyledItemDelegate::setModelData(editor, model, index);
    }
}

void EnumAttributeDelegate::updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    Q_UNUSED(index);
    editor->setGeometry(option.rect);
}

// ========== ExtraAttrUI Implementation ==========

// Singleton instance
ExtraAttrUI* ExtraAttrUI::s_instance = nullptr;

ExtraAttrUI::ExtraAttrUI(QWidget* parent)
    : QMainWindow(parent)
    , m_scanner(new ExtraAttrScanner())
{
    setupUI();
}

ExtraAttrUI::~ExtraAttrUI()
{
    s_instance = nullptr;
}

ExtraAttrUI* ExtraAttrUI::instance()
{
    if (!s_instance) {
        // Get Maya main window as QWidget
        QWidget* mayaMainWindow = MQtUtil::mainWindow();
        s_instance = new ExtraAttrUI(mayaMainWindow);
    }
    return s_instance;
}

void ExtraAttrUI::destroyInstance()
{
    if (s_instance) {
        delete s_instance;
        s_instance = nullptr;
    }
}

void ExtraAttrUI::setupUI()
{
    setWindowTitle("Extra Attribute Editor");
    setMinimumSize(1200, 600);

    // Central widget
    QWidget* centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    QVBoxLayout* mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setContentsMargins(5, 5, 5, 5);
    mainLayout->setSpacing(3);  // Minimize space between widgets

    // Toolbar
    QWidget* toolbar = createToolBar();
    toolbar->setMaximumHeight(30);  // Limit toolbar maximum height
    mainLayout->addWidget(toolbar);

    // Splitter (horizontal split)
    QSplitter* splitter = new QSplitter(Qt::Horizontal, this);

    // Left side: Extra Attribute list
    QWidget* leftWidget = new QWidget();
    QVBoxLayout* leftLayout = new QVBoxLayout(leftWidget);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(2);  // Minimize space

    QLabel* leftLabel = new QLabel("Extra Attributes:");
    QFont labelFont = leftLabel->font();
    labelFont.setPointSize(labelFont.pointSize() - 1);  // Make font size smaller
    leftLabel->setFont(labelFont);
    leftLabel->setMaximumHeight(20);  // Limit label height
    leftLayout->addWidget(leftLabel);

    // Filter input field (left side)
    m_attributeFilterLineEdit = new QLineEdit();
    m_attributeFilterLineEdit->setPlaceholderText("Filter attributes...");
    m_attributeFilterLineEdit->setFixedHeight(24);
    leftLayout->addWidget(m_attributeFilterLineEdit);

    m_attributeTableView = new QTableView();
    m_attributeModel = new ExtraAttrModel(this);

    // Use proxy model for filtering and sorting
    m_attributeProxyModel = new QSortFilterProxyModel(this);
    m_attributeProxyModel->setSourceModel(m_attributeModel);
    m_attributeProxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    m_attributeProxyModel->setFilterKeyColumn(ExtraAttrModel::COL_ATTR_NAME);

    m_attributeTableView->setModel(m_attributeProxyModel);
    m_attributeTableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_attributeTableView->setSelectionMode(QAbstractItemView::SingleSelection);
    m_attributeTableView->setSortingEnabled(true);
    m_attributeTableView->setContextMenuPolicy(Qt::CustomContextMenu);
    m_attributeTableView->horizontalHeader()->setStretchLastSection(true);
    m_attributeTableView->horizontalHeader()->setDefaultSectionSize(100);  // Optimize column width
    m_attributeTableView->horizontalHeader()->setMinimumHeight(20);  // Minimize header height
    m_attributeTableView->verticalHeader()->setVisible(false);
    m_attributeTableView->verticalHeader()->setDefaultSectionSize(20);  // Reduce row height

    leftLayout->addWidget(m_attributeTableView);
    splitter->addWidget(leftWidget);

    // Right side: Node list
    QWidget* rightWidget = new QWidget();
    QVBoxLayout* rightLayout = new QVBoxLayout(rightWidget);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(2);  // Minimize space

    QLabel* rightLabel = new QLabel("Nodes using selected attribute:");
    rightLabel->setFont(labelFont);  // Same font size as left side
    rightLabel->setMaximumHeight(20);  // Limit label height
    rightLayout->addWidget(rightLabel);

    // Filter input field and column selection radio buttons (right side)
    QHBoxLayout* nodeFilterLayout = new QHBoxLayout();
    nodeFilterLayout->setContentsMargins(0, 0, 0, 0);
    nodeFilterLayout->setSpacing(5);

    m_nodeFilterLineEdit = new QLineEdit();
    m_nodeFilterLineEdit->setPlaceholderText("Filter nodes...");
    m_nodeFilterLineEdit->setFixedHeight(24);
    nodeFilterLayout->addWidget(m_nodeFilterLineEdit);

    // Radio buttons
    m_filterNodeNameRadio = new QRadioButton("Node Name");
    m_filterNodeTypeRadio = new QRadioButton("Node Type");
    m_filterValueRadio = new QRadioButton("Value");

    m_filterNodeNameRadio->setChecked(true);  // Default is Node Name

    // Make font size smaller
    QFont radioFont = m_filterNodeNameRadio->font();
    radioFont.setPointSize(radioFont.pointSize() - 1);
    m_filterNodeNameRadio->setFont(radioFont);
    m_filterNodeTypeRadio->setFont(radioFont);
    m_filterValueRadio->setFont(radioFont);

    m_filterButtonGroup = new QButtonGroup(this);
    m_filterButtonGroup->addButton(m_filterNodeNameRadio, NodeAttributeModel::COL_NODE_NAME);
    m_filterButtonGroup->addButton(m_filterNodeTypeRadio, NodeAttributeModel::COL_NODE_TYPE);
    m_filterButtonGroup->addButton(m_filterValueRadio, NodeAttributeModel::COL_VALUE);

    nodeFilterLayout->addWidget(m_filterNodeNameRadio);
    nodeFilterLayout->addWidget(m_filterNodeTypeRadio);
    nodeFilterLayout->addWidget(m_filterValueRadio);

    rightLayout->addLayout(nodeFilterLayout);

    m_nodeTableView = new QTableView();
    m_nodeModel = new NodeAttributeModel(this);

    // Use proxy model for filtering and sorting
    m_nodeProxyModel = new QSortFilterProxyModel(this);
    m_nodeProxyModel->setSourceModel(m_nodeModel);
    m_nodeProxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    m_nodeProxyModel->setFilterKeyColumn(NodeAttributeModel::COL_NODE_NAME);  // Default is Node Name

    m_nodeTableView->setModel(m_nodeProxyModel);
    m_nodeTableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_nodeTableView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_nodeTableView->setSortingEnabled(true);  // Enable sorting
    m_nodeTableView->setContextMenuPolicy(Qt::CustomContextMenu);
    m_nodeTableView->horizontalHeader()->setStretchLastSection(true);
    m_nodeTableView->horizontalHeader()->setDefaultSectionSize(100);  // Optimize column width
    m_nodeTableView->horizontalHeader()->setMinimumHeight(20);  // Minimize header height
    m_nodeTableView->verticalHeader()->setVisible(false);
    m_nodeTableView->verticalHeader()->setDefaultSectionSize(20);  // Reduce row height

    // Set custom delegate for enum attributes
    m_nodeTableView->setItemDelegate(new EnumAttributeDelegate(this));

    rightLayout->addWidget(m_nodeTableView);
    splitter->addWidget(rightWidget);

    // Splitter size ratio (50/50)
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 1);

    mainLayout->addWidget(splitter);

    // Signal/slot connections
    connect(m_scanButton, &QPushButton::clicked, this, &ExtraAttrUI::onScanButtonClicked);

    connect(m_attributeFilterLineEdit, &QLineEdit::textChanged, this, &ExtraAttrUI::onAttributeFilterChanged);
    connect(m_nodeFilterLineEdit, &QLineEdit::textChanged, this, &ExtraAttrUI::onNodeFilterChanged);
    connect(m_filterButtonGroup, &QButtonGroup::buttonClicked,
            [this]() { onNodeFilterChanged(m_nodeFilterLineEdit->text()); });

    connect(m_attributeTableView->selectionModel(), &QItemSelectionModel::currentChanged,
            this, &ExtraAttrUI::onAttributeSelectionChanged);

    connect(m_attributeTableView, &QTableView::customContextMenuRequested,
            this, &ExtraAttrUI::onAttributeContextMenu);

    connect(m_nodeTableView, &QTableView::customContextMenuRequested,
            this, &ExtraAttrUI::onNodeContextMenu);

    connect(m_nodeTableView->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &ExtraAttrUI::onNodeSelectionChanged);

    connect(m_nodeModel, &NodeAttributeModel::valueChanged,
            this, &ExtraAttrUI::onNodeValueChanged);
}

QWidget* ExtraAttrUI::createToolBar()
{
    QWidget* toolbar = new QWidget();
    QHBoxLayout* layout = new QHBoxLayout(toolbar);
    layout->setContentsMargins(5, 2, 5, 2);  // Minimize top/bottom margins
    layout->setSpacing(5);  // Reduce space between widgets

    m_scanButton = new QPushButton("Scan Scene");
    m_scanButton->setFixedSize(80, 24);  // Set height to make smaller
    layout->addWidget(m_scanButton);

    layout->addStretch();

    m_statsLabel = new QLabel("No data");
    layout->addWidget(m_statsLabel);

    return toolbar;
}

void ExtraAttrUI::showUI()
{
    show();
    raise();
    activateWindow();
}

void ExtraAttrUI::closeUI()
{
    close();
}

void ExtraAttrUI::closeEvent(QCloseEvent* event)
{
    // Handler when closing window
    QMainWindow::closeEvent(event);
}

void ExtraAttrUI::onScanButtonClicked()
{
    // Preserve current selection
    QString currentAttr = m_currentAttributeName;

    // Show progress dialog
    QProgressDialog progress("Scanning scene for extra attributes...", "Cancel", 0, 0, this);
    progress.setWindowModality(Qt::WindowModal);
    progress.setMinimumDuration(500);
    progress.setValue(0);

    QApplication::processEvents();

    // Scan scene
    bool success = m_scanner->scanScene();

    if (!success) {
        QMessageBox::warning(this, "Scan Error", "Failed to scan scene for extra attributes.");
        return;
    }

    // Load data into model
    m_attributeModel->loadFromScanner(*m_scanner);

    // Update statistics
    updateStatistics();

    // Set default sort to ascending order by Name column
    m_attributeTableView->sortByColumn(ExtraAttrModel::COL_ATTR_NAME, Qt::AscendingOrder);

    // Restore selection if attribute still exists
    if (!currentAttr.isEmpty()) {
        for (int i = 0; i < m_attributeProxyModel->rowCount(); ++i) {
            QModelIndex proxyIndex = m_attributeProxyModel->index(i, 0);
            QModelIndex sourceIndex = m_attributeProxyModel->mapToSource(proxyIndex);
            if (m_attributeModel->getAttributeName(sourceIndex.row()) == currentAttr) {
                m_attributeTableView->selectRow(i);
                break;
            }
        }
    } else {
        // Clear right side table if no selection
        m_nodeModel->clear();
        m_currentAttributeName.clear();
    }

    progress.setValue(100);
}

void ExtraAttrUI::onAttributeSelectionChanged(const QModelIndex& current, const QModelIndex& previous)
{
    Q_UNUSED(previous);

    if (!current.isValid()) {
        m_nodeModel->clear();
        return;
    }

    // Convert proxy model index to source model index
    QModelIndex sourceIndex = m_attributeProxyModel->mapToSource(current);

    // Get selected attribute name
    QString attrName = m_attributeModel->getAttributeName(sourceIndex.row());
    if (attrName.isEmpty()) {
        return;
    }

    m_currentAttributeName = attrName;

    // Get list of nodes with this attribute
    MString mayaAttrName(attrName.toUtf8().constData());
    std::vector<NodeAttributeValue> nodeValues = m_scanner->getNodesWithAttribute(mayaAttrName);

    // Set to node model
    m_nodeModel->setNodeValues(attrName, nodeValues);

    // Set default sort to ascending order by NodeName column
    m_nodeTableView->sortByColumn(NodeAttributeModel::COL_NODE_NAME, Qt::AscendingOrder);
}

void ExtraAttrUI::onSearchTextChanged(const QString& text)
{
    // Simple filtering (better to use proxy model in the future)
    // Currently handled by prompting rescan
    if (text.isEmpty()) {
        return;
    }

    // TODO: Implement filtering with proxy model
    // Currently a stub for search function
}

void ExtraAttrUI::onNodeValueChanged(const QString& nodeName, const QString& attrName, const QString& newValue)
{
    // Change attribute value using Maya API
    bool success = setAttributeValue(nodeName, attrName, newValue);

    if (success) {
        MGlobal::displayInfo(MString("Updated ") + nodeName.toUtf8().constData() + "." +
                             attrName.toUtf8().constData() + " = " + newValue.toUtf8().constData());
    } else {
        QMessageBox::warning(this, "Edit Error",
                             QString("Failed to update attribute: %1.%2").arg(nodeName).arg(attrName));
    }
}

void ExtraAttrUI::onAttributeContextMenu(const QPoint& pos)
{
    QModelIndex index = m_attributeTableView->indexAt(pos);
    if (!index.isValid()) {
        return;
    }

    QMenu menu(this);
    QAction* addAction = menu.addAction("Add Attribute to Selected Nodes...");
    QAction* deleteAction = menu.addAction("Delete Attribute from All Nodes...");

    QAction* selectedAction = menu.exec(m_attributeTableView->viewport()->mapToGlobal(pos));

    if (selectedAction == addAction) {
        onAddAttribute();
    } else if (selectedAction == deleteAction) {
        onDeleteAttribute();
    }
}

void ExtraAttrUI::onNodeContextMenu(const QPoint& pos)
{
    QModelIndex index = m_nodeTableView->indexAt(pos);
    if (!index.isValid()) {
        return;
    }

    // Get selected node name
    QModelIndex sourceIndex = m_nodeProxyModel->mapToSource(index);
    QString nodeName = m_nodeModel->getNodeName(sourceIndex.row());

    QMenu menu(this);
    QAction* selectAction = menu.addAction("Select Node in Maya");

    // Add polygon selection option if node is a material
    QAction* selectPolygonsAction = nullptr;
    if (isShadingNode(nodeName)) {
        selectPolygonsAction = menu.addAction("Select Assigned Polygons");
    }

    QAction* deleteAction = menu.addAction("Delete Attribute from This Node...");
    menu.addSeparator();
    QAction* batchAction = menu.addAction("Batch Edit Selected Nodes...");

    QAction* selectedAction = menu.exec(m_nodeTableView->viewport()->mapToGlobal(pos));

    if (selectedAction == selectAction) {
        onSelectNode();
    } else if (selectedAction == selectPolygonsAction && selectPolygonsAction) {
        onSelectAssignedPolygons();
    } else if (selectedAction == deleteAction) {
        onDeleteAttribute();
    } else if (selectedAction == batchAction) {
        onBatchEdit();
    }
}

void ExtraAttrUI::onDeleteAttribute()
{
    QModelIndexList selectedRows = m_nodeTableView->selectionModel()->selectedRows();
    if (selectedRows.isEmpty()) {
        QMessageBox::information(this, "Delete Attribute", "Please select at least one node.");
        return;
    }

    if (m_currentAttributeName.isEmpty()) {
        return;
    }

    QMessageBox::StandardButton reply = QMessageBox::question(
        this, "Delete Attribute",
        QString("Are you sure you want to delete attribute '%1' from %2 node(s)?")
            .arg(m_currentAttributeName)
            .arg(selectedRows.size()),
        QMessageBox::Yes | QMessageBox::No);

    if (reply != QMessageBox::Yes) {
        return;
    }

    int successCount = 0;
    for (const QModelIndex& index : selectedRows) {
        QModelIndex sourceIndex = m_nodeProxyModel->mapToSource(index);
        QString nodeName = m_nodeModel->getNodeName(sourceIndex.row());
        if (deleteAttribute(nodeName, m_currentAttributeName)) {
            successCount++;
        }
    }

    QMessageBox::information(this, "Delete Complete",
                             QString("Deleted attribute from %1 of %2 nodes.")
                                 .arg(successCount)
                                 .arg(selectedRows.size()));

    onScanButtonClicked();
}

void ExtraAttrUI::onAddAttribute()
{
    bool ok;
    QString attrName = QInputDialog::getText(this, "Add Attribute",
                                             "Attribute name:", QLineEdit::Normal,
                                             "", &ok);
    if (!ok || attrName.isEmpty()) {
        return;
    }

    QStringList types;
    types << "double" << "int" << "bool" << "string" << "enum";
    QString attrType = QInputDialog::getItem(this, "Add Attribute",
                                             "Attribute type:", types, 0, false, &ok);
    if (!ok) {
        return;
    }

    MSelectionList selList;
    MGlobal::getActiveSelectionList(selList);

    if (selList.length() == 0) {
        QMessageBox::information(this, "Add Attribute", "Please select at least one node in Maya.");
        return;
    }

    int successCount = 0;
    for (unsigned int i = 0; i < selList.length(); ++i) {
        MObject node;
        selList.getDependNode(i, node);

        MFnDependencyNode fnDep(node);
        QString nodeName = QString::fromUtf8(fnDep.name().asChar());

        if (addAttribute(nodeName, attrName, attrType)) {
            successCount++;
        }
    }

    QMessageBox::information(this, "Add Complete",
                             QString("Added attribute to %1 of %2 nodes.")
                                 .arg(successCount)
                                 .arg(selList.length()));

    onScanButtonClicked();
}

void ExtraAttrUI::onBatchEdit()
{
    QModelIndexList selectedRows = m_nodeTableView->selectionModel()->selectedRows();
    if (selectedRows.isEmpty()) {
        QMessageBox::information(this, "Batch Edit", "Please select at least one node.");
        return;
    }

    if (m_currentAttributeName.isEmpty()) {
        return;
    }

    bool ok;
    QString newValue = QInputDialog::getText(this, "Batch Edit",
                                             QString("New value for attribute '%1':").arg(m_currentAttributeName),
                                             QLineEdit::Normal, "", &ok);
    if (!ok) {
        return;
    }

    int successCount = 0;
    for (const QModelIndex& index : selectedRows) {
        QModelIndex sourceIndex = m_nodeProxyModel->mapToSource(index);
        QString nodeName = m_nodeModel->getNodeName(sourceIndex.row());
        if (setAttributeValue(nodeName, m_currentAttributeName, newValue)) {
            successCount++;
        }
    }

    QMessageBox::information(this, "Batch Edit Complete",
                             QString("Updated attribute for %1 of %2 nodes.")
                                 .arg(successCount)
                                 .arg(selectedRows.size()));

    onScanButtonClicked();
}

void ExtraAttrUI::onSelectNode()
{
    QModelIndex index = m_nodeTableView->currentIndex();
    if (!index.isValid()) {
        return;
    }

    QModelIndex sourceIndex = m_nodeProxyModel->mapToSource(index);
    QString nodeName = m_nodeModel->getNodeName(sourceIndex.row());
    selectNodeInMaya(nodeName);
}

void ExtraAttrUI::updateStatistics()
{
    int totalAttrs, totalNodes;
    m_scanner->getStatistics(totalAttrs, totalNodes);

    m_statsLabel->setText(QString("Total: %1 attributes in %2 nodes")
                              .arg(totalAttrs)
                              .arg(totalNodes));
}

bool ExtraAttrUI::setAttributeValue(const QString& nodeName, const QString& attrName, const QString& value)
{
    MStatus status;
    MFnDependencyNode fnDep;

    // Get node
    MString mNodeName(nodeName.toUtf8().constData());
    if (!MayaUtils::getDependencyNodeFromName(mNodeName, fnDep)) {
        return false;
    }

    // Get attribute
    MString mAttrName(attrName.toUtf8().constData());
    MObject attr = fnDep.attribute(mAttrName, &status);
    if (status != MS::kSuccess || attr.isNull()) {
        return false;
    }

    MPlug plug = fnDep.findPlug(attr, &status);
    if (status != MS::kSuccess) {
        return false;
    }

    // Set value using utility function
    MString mValue(value.toUtf8().constData());
    return MayaUtils::setAttributeValueFromString(plug, attr, mValue);
}

bool ExtraAttrUI::deleteAttribute(const QString& nodeName, const QString& attrName)
{
    MStatus status;
    MFnDependencyNode fnDep;

    // Get node
    MString mNodeName(nodeName.toUtf8().constData());
    if (!MayaUtils::getDependencyNodeFromName(mNodeName, fnDep)) {
        return false;
    }

    // Get attribute
    MString mAttrName(attrName.toUtf8().constData());
    MObject attr = fnDep.attribute(mAttrName, &status);
    if (status != MS::kSuccess || attr.isNull()) {
        return false;
    }

    status = fnDep.removeAttribute(attr);
    return (status == MS::kSuccess);
}

bool ExtraAttrUI::addAttribute(const QString& nodeName, const QString& attrName, const QString& attrType)
{
    MStatus status;
    MFnDependencyNode fnDep;

    // Get node
    MString mNodeName(nodeName.toUtf8().constData());
    if (!MayaUtils::getDependencyNodeFromName(mNodeName, fnDep)) {
        return false;
    }

    MString mayaAttrName(attrName.toUtf8().constData());
    MObject attr;

    if (attrType == "double" || attrType == "float") {
        MFnNumericAttribute nAttr;
        attr = nAttr.create(mayaAttrName, mayaAttrName, MFnNumericData::kDouble, 0.0, &status);
        if (status == MS::kSuccess) {
            nAttr.setKeyable(true);
        }
    } else if (attrType == "int") {
        MFnNumericAttribute nAttr;
        attr = nAttr.create(mayaAttrName, mayaAttrName, MFnNumericData::kInt, 0, &status);
        if (status == MS::kSuccess) {
            nAttr.setKeyable(true);
        }
    } else if (attrType == "bool") {
        MFnNumericAttribute nAttr;
        attr = nAttr.create(mayaAttrName, mayaAttrName, MFnNumericData::kBoolean, false, &status);
        if (status == MS::kSuccess) {
            nAttr.setKeyable(true);
        }
    } else if (attrType == "string") {
        MFnTypedAttribute tAttr;
        attr = tAttr.create(mayaAttrName, mayaAttrName, MFnData::kString, &status);
    }

    if (status != MS::kSuccess || attr.isNull()) {
        return false;
    }

    status = fnDep.addAttribute(attr);
    return (status == MS::kSuccess);
}

bool ExtraAttrUI::selectNodeInMaya(const QString& nodeName)
{
    MStatus status;

    MSelectionList selList;
    status = selList.add(nodeName.toUtf8().constData());
    if (status != MS::kSuccess) {
        return false;
    }

    // Check if the node is a shading node (material, texture, etc.)
    MObject nodeObj;
    status = selList.getDependNode(0, nodeObj);
    if (status != MS::kSuccess || nodeObj.isNull()) {
        return false;
    }

    MFnDependencyNode fnDep(nodeObj, &status);
    if (status != MS::kSuccess) {
        return false;
    }

    // Check if node is a shading engine or material
    bool isShadingNode = false;
    MString nodeType = fnDep.typeName(&status);

    // Debug: Print node type
    MString debugMsg = "ExtraAttrUI: Node '";
    debugMsg += nodeName.toUtf8().constData();
    debugMsg += "' has type: ";
    debugMsg += nodeType;
    MGlobal::displayInfo(debugMsg);

    if (status == MS::kSuccess) {
        // Check if it's a shading-related node
        // Include common Maya material types
        if (nodeType == "lambert" ||
            nodeType == "blinn" ||
            nodeType == "phong" ||
            nodeType == "phongE" ||
            nodeType == "anisotropic" ||
            nodeType == "standardSurface" ||
            nodeType == "aiStandardSurface" ||
            nodeType == "shadingEngine" ||
            nodeType.indexW("shader") >= 0 ||
            nodeType.indexW("material") >= 0 ||
            nodeType.indexW("texture") >= 0) {
            isShadingNode = true;
            MGlobal::displayInfo("ExtraAttrUI: Detected as shading node");
        } else {
            MGlobal::displayInfo("ExtraAttrUI: Detected as regular node");
        }
    }

    if (isShadingNode) {
        // For shading nodes, enable "Assigned Materials" display and select objects with this material
        MString pythonCmd = "import maya.cmds as mc\n";
        pythonCmd += "node = '" + MString(nodeName.toUtf8().constData()) + "'\n";
        pythonCmd += "try:\n";
        pythonCmd += "    # First, enable 'Assigned Materials' display in all outliners\n";
        pythonCmd += "    outliner_panels = mc.getPanel(type='outlinerPanel')\n";
        pythonCmd += "    if outliner_panels:\n";
        pythonCmd += "        for panel in outliner_panels:\n";
        pythonCmd += "            outliner = mc.outlinerPanel(panel, query=True, outlinerEditor=True)\n";
        pythonCmd += "            if outliner:\n";
        pythonCmd += "                mc.outlinerEditor(outliner, edit=True, showAssignedMaterials=True)\n";
        pythonCmd += "    # Find objects assigned to this material\n";
        pythonCmd += "    objects = []\n";
        pythonCmd += "    shading_engines = []\n";
        pythonCmd += "    if mc.objectType(node) == 'shadingEngine':\n";
        pythonCmd += "        shading_engines = [node]\n";
        pythonCmd += "    else:\n";
        pythonCmd += "        connections = mc.listConnections(node, type='shadingEngine', destination=True) or []\n";
        pythonCmd += "        shading_engines = list(set(connections))\n";
        pythonCmd += "    for sg in shading_engines:\n";
        pythonCmd += "        try:\n";
        pythonCmd += "            members = mc.sets(sg, query=True) or []\n";
        pythonCmd += "            for member in members:\n";
        pythonCmd += "                if mc.objectType(member, isAType='shape'):\n";
        pythonCmd += "                    transforms = mc.listRelatives(member, parent=True, fullPath=True) or []\n";
        pythonCmd += "                    objects.extend(transforms)\n";
        pythonCmd += "                elif mc.objectType(member, isAType='transform'):\n";
        pythonCmd += "                    objects.append(member)\n";
        pythonCmd += "        except Exception as e:\n";
        pythonCmd += "            print('Error processing shading engine: ' + str(e))\n";
        pythonCmd += "    # Select objects and expand in outliner\n";
        pythonCmd += "    if objects:\n";
        pythonCmd += "        mc.select(objects, replace=True)\n";
        pythonCmd += "        # Expand outliner to show selected objects\n";
        pythonCmd += "        outliner_panels = mc.getPanel(type='outlinerPanel')\n";
        pythonCmd += "        if outliner_panels:\n";
        pythonCmd += "            for panel in outliner_panels:\n";
        pythonCmd += "                outliner = mc.outlinerPanel(panel, query=True, outlinerEditor=True)\n";
        pythonCmd += "                if outliner:\n";
        pythonCmd += "                    mc.outlinerEditor(outliner, edit=True, showSelected=True)\n";
        pythonCmd += "        print('Selected ' + str(len(objects)) + ' objects with material: ' + node)\n";
        pythonCmd += "    else:\n";
        pythonCmd += "        # If no objects found, just select the material itself\n";
        pythonCmd += "        mc.select(node, replace=True)\n";
        pythonCmd += "        print('No objects found for material: ' + node)\n";
        pythonCmd += "except Exception as e:\n";
        pythonCmd += "    print('Error selecting objects for material: ' + str(e))\n";

        MGlobal::executePythonCommand(pythonCmd);
    } else {
        // For regular nodes, select directly
        status = MGlobal::setActiveSelectionList(selList);
        if (status != MS::kSuccess) {
            return false;
        }

        // Expand outliner hierarchy to show selected node
        MString pythonCmd =
            "import maya.cmds as mc\n"
            "outliner_panels = mc.getPanel(type='outlinerPanel')\n"
            "if outliner_panels:\n"
            "    for panel in outliner_panels:\n"
            "        outliner = mc.outlinerPanel(panel, query=True, outlinerEditor=True)\n"
            "        if outliner:\n"
            "            mc.outlinerEditor(outliner, edit=True, showSelected=True)\n";
        MGlobal::executePythonCommand(pythonCmd);
    }

    return true;
}

bool ExtraAttrUI::isShadingNode(const QString& nodeName)
{
    MStatus status;
    MSelectionList selList;
    status = selList.add(nodeName.toUtf8().constData());
    if (status != MS::kSuccess) {
        return false;
    }

    MObject nodeObj;
    status = selList.getDependNode(0, nodeObj);
    if (status != MS::kSuccess || nodeObj.isNull()) {
        return false;
    }

    MFnDependencyNode fnDep(nodeObj, &status);
    if (status != MS::kSuccess) {
        return false;
    }

    MString nodeType = fnDep.typeName(&status);
    if (status != MS::kSuccess) {
        return false;
    }

    // Check if it's a shading-related node
    if (nodeType == "lambert" ||
        nodeType == "blinn" ||
        nodeType == "phong" ||
        nodeType == "phongE" ||
        nodeType == "anisotropic" ||
        nodeType == "standardSurface" ||
        nodeType == "aiStandardSurface" ||
        nodeType == "shadingEngine" ||
        nodeType.indexW("shader") >= 0 ||
        nodeType.indexW("material") >= 0 ||
        nodeType.indexW("texture") >= 0) {
        return true;
    }

    return false;
}

bool ExtraAttrUI::selectPolygonsWithMaterial(const QString& materialName)
{
    // Use Python to select polygons assigned to this material
    // This handles face assignments properly, supporting multiple materials per mesh
    MString pythonCmd = "import maya.cmds as mc\n";
    pythonCmd += "material = '" + MString(materialName.toUtf8().constData()) + "'\n";
    pythonCmd += "try:\n";
    pythonCmd += "    # Find shading engines connected to this material\n";
    pythonCmd += "    shading_engines = []\n";
    pythonCmd += "    if mc.objectType(material) == 'shadingEngine':\n";
    pythonCmd += "        shading_engines = [material]\n";
    pythonCmd += "    else:\n";
    pythonCmd += "        connections = mc.listConnections(material, type='shadingEngine', destination=True) or []\n";
    pythonCmd += "        shading_engines = list(set(connections))\n";
    pythonCmd += "    \n";
    pythonCmd += "    if not shading_engines:\n";
    pythonCmd += "        print('No shading engines found for material: ' + material)\n";
    pythonCmd += "        mc.select(clear=True)\n";
    pythonCmd += "    else:\n";
    pythonCmd += "        # Get all faces assigned to these shading engines\n";
    pythonCmd += "        face_list = []\n";
    pythonCmd += "        for sg in shading_engines:\n";
    pythonCmd += "            try:\n";
    pythonCmd += "                members = mc.sets(sg, query=True) or []\n";
    pythonCmd += "                for member in members:\n";
    pythonCmd += "                    # Check if it's a face component (e.g., 'pCube1.f[0:5]')\n";
    pythonCmd += "                    if '.f[' in member:\n";
    pythonCmd += "                        face_list.append(member)\n";
    pythonCmd += "                    # If it's a whole mesh shape, convert to all faces\n";
    pythonCmd += "                    elif mc.objectType(member, isAType='mesh'):\n";
    pythonCmd += "                        # Get the number of faces\n";
    pythonCmd += "                        face_count = mc.polyEvaluate(member, face=True)\n";
    pythonCmd += "                        if face_count > 0:\n";
    pythonCmd += "                            face_list.append(member + '.f[0:' + str(face_count-1) + ']')\n";
    pythonCmd += "                    # If it's a transform node, get its shape and convert to faces\n";
    pythonCmd += "                    elif mc.objectType(member, isAType='transform'):\n";
    pythonCmd += "                        shapes = mc.listRelatives(member, shapes=True, type='mesh') or []\n";
    pythonCmd += "                        for shape in shapes:\n";
    pythonCmd += "                            face_count = mc.polyEvaluate(shape, face=True)\n";
    pythonCmd += "                            if face_count > 0:\n";
    pythonCmd += "                                face_list.append(shape + '.f[0:' + str(face_count-1) + ']')\n";
    pythonCmd += "            except Exception as e:\n";
    pythonCmd += "                print('Error processing shading engine ' + sg + ': ' + str(e))\n";
    pythonCmd += "        \n";
    pythonCmd += "        if face_list:\n";
    pythonCmd += "            mc.select(face_list, replace=True)\n";
    pythonCmd += "            print('Selected ' + str(len(face_list)) + ' face component(s) with material: ' + material)\n";
    pythonCmd += "        else:\n";
    pythonCmd += "            print('No polygons found for material: ' + material)\n";
    pythonCmd += "            mc.select(clear=True)\n";
    pythonCmd += "except Exception as e:\n";
    pythonCmd += "    print('Error selecting polygons for material: ' + str(e))\n";
    pythonCmd += "    mc.select(clear=True)\n";

    MStatus status = MGlobal::executePythonCommand(pythonCmd);
    return (status == MS::kSuccess);
}

void ExtraAttrUI::onSelectAssignedPolygons()
{
    QModelIndexList selectedRows = m_nodeTableView->selectionModel()->selectedRows();
    if (selectedRows.isEmpty()) {
        QMessageBox::information(this, "Select Polygons", "Please select at least one material node.");
        return;
    }

    // Collect all material names
    QStringList materialNames;
    for (const QModelIndex& index : selectedRows) {
        QModelIndex sourceIndex = m_nodeProxyModel->mapToSource(index);
        QString nodeName = m_nodeModel->getNodeName(sourceIndex.row());
        if (isShadingNode(nodeName)) {
            materialNames.append(nodeName);
        }
    }

    if (materialNames.isEmpty()) {
        QMessageBox::information(this, "Select Polygons", "No material nodes selected.");
        return;
    }

    // Build Python command to select polygons for all materials
    MString pythonCmd = "import maya.cmds as mc\n";
    pythonCmd += "materials = [";
    for (int i = 0; i < materialNames.size(); ++i) {
        pythonCmd += "'" + MString(materialNames[i].toUtf8().constData()) + "'";
        if (i < materialNames.size() - 1) {
            pythonCmd += ", ";
        }
    }
    pythonCmd += "]\n";
    pythonCmd += "try:\n";
    pythonCmd += "    all_faces = []\n";
    pythonCmd += "    for material in materials:\n";
    pythonCmd += "        # Find shading engines connected to this material\n";
    pythonCmd += "        shading_engines = []\n";
    pythonCmd += "        if mc.objectType(material) == 'shadingEngine':\n";
    pythonCmd += "            shading_engines = [material]\n";
    pythonCmd += "        else:\n";
    pythonCmd += "            connections = mc.listConnections(material, type='shadingEngine', destination=True) or []\n";
    pythonCmd += "            shading_engines = list(set(connections))\n";
    pythonCmd += "        \n";
    pythonCmd += "        # Get all faces assigned to these shading engines\n";
    pythonCmd += "        for sg in shading_engines:\n";
    pythonCmd += "            try:\n";
    pythonCmd += "                members = mc.sets(sg, query=True) or []\n";
    pythonCmd += "                for member in members:\n";
    pythonCmd += "                    # Check if it's a face component (e.g., 'pCube1.f[0:5]')\n";
    pythonCmd += "                    if '.f[' in member:\n";
    pythonCmd += "                        all_faces.append(member)\n";
    pythonCmd += "                    # If it's a whole mesh shape, convert to all faces\n";
    pythonCmd += "                    elif mc.objectType(member, isAType='mesh'):\n";
    pythonCmd += "                        face_count = mc.polyEvaluate(member, face=True)\n";
    pythonCmd += "                        if face_count > 0:\n";
    pythonCmd += "                            all_faces.append(member + '.f[0:' + str(face_count-1) + ']')\n";
    pythonCmd += "                    # If it's a transform node, get its shape and convert to faces\n";
    pythonCmd += "                    elif mc.objectType(member, isAType='transform'):\n";
    pythonCmd += "                        shapes = mc.listRelatives(member, shapes=True, type='mesh') or []\n";
    pythonCmd += "                        for shape in shapes:\n";
    pythonCmd += "                            face_count = mc.polyEvaluate(shape, face=True)\n";
    pythonCmd += "                            if face_count > 0:\n";
    pythonCmd += "                                all_faces.append(shape + '.f[0:' + str(face_count-1) + ']')\n";
    pythonCmd += "            except Exception as e:\n";
    pythonCmd += "                print('Error processing shading engine ' + sg + ': ' + str(e))\n";
    pythonCmd += "    \n";
    pythonCmd += "    if all_faces:\n";
    pythonCmd += "        mc.select(all_faces, replace=True)\n";
    pythonCmd += "        print('Selected ' + str(len(all_faces)) + ' face component(s) from ' + str(len(materials)) + ' material(s)')\n";
    pythonCmd += "    else:\n";
    pythonCmd += "        print('No polygons found for selected materials')\n";
    pythonCmd += "        mc.select(clear=True)\n";
    pythonCmd += "except Exception as e:\n";
    pythonCmd += "    print('Error selecting polygons for materials: ' + str(e))\n";
    pythonCmd += "    mc.select(clear=True)\n";

    MStatus status = MGlobal::executePythonCommand(pythonCmd);
    if (status != MS::kSuccess) {
        QMessageBox::warning(this, "Select Polygons",
                           QString("Failed to select polygons for selected materials."));
    }
}

void ExtraAttrUI::onAttributeFilterChanged(const QString& text)
{
    m_attributeProxyModel->setFilterFixedString(text);
}

void ExtraAttrUI::onNodeFilterChanged(const QString& text)
{
    int filterColumn = m_filterButtonGroup->checkedId();
    m_nodeProxyModel->setFilterKeyColumn(filterColumn);
    m_nodeProxyModel->setFilterFixedString(text);
}

void ExtraAttrUI::onNodeSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected)
{
    Q_UNUSED(deselected);

    QModelIndexList selectedRows = m_nodeTableView->selectionModel()->selectedRows();
    if (selectedRows.isEmpty()) {
        return;
    }

    // Collect all selected node names
    QStringList nodeNames;
    QStringList materialNames;

    for (const QModelIndex& index : selectedRows) {
        QModelIndex sourceIndex = m_nodeProxyModel->mapToSource(index);
        QString nodeName = m_nodeModel->getNodeName(sourceIndex.row());
        if (!nodeName.isEmpty()) {
            nodeNames.append(nodeName);
            if (isShadingNode(nodeName)) {
                materialNames.append(nodeName);
            }
        }
    }

    if (nodeNames.isEmpty()) {
        return;
    }

    // If all selected nodes are materials, select objects assigned to those materials
    if (!materialNames.isEmpty() && materialNames.size() == nodeNames.size()) {
        // All selected nodes are materials - select their assigned objects
        MString pythonCmd = "import maya.cmds as mc\n";
        pythonCmd += "materials = [";
        for (int i = 0; i < materialNames.size(); ++i) {
            pythonCmd += "'" + MString(materialNames[i].toUtf8().constData()) + "'";
            if (i < materialNames.size() - 1) {
                pythonCmd += ", ";
            }
        }
        pythonCmd += "]\n";
        pythonCmd += "try:\n";
        pythonCmd += "    # First, enable 'Assigned Materials' display in all outliners\n";
        pythonCmd += "    outliner_panels = mc.getPanel(type='outlinerPanel')\n";
        pythonCmd += "    if outliner_panels:\n";
        pythonCmd += "        for panel in outliner_panels:\n";
        pythonCmd += "            outliner = mc.outlinerPanel(panel, query=True, outlinerEditor=True)\n";
        pythonCmd += "            if outliner:\n";
        pythonCmd += "                mc.outlinerEditor(outliner, edit=True, showAssignedMaterials=True)\n";
        pythonCmd += "    # Find objects assigned to these materials\n";
        pythonCmd += "    objects = []\n";
        pythonCmd += "    for material in materials:\n";
        pythonCmd += "        shading_engines = []\n";
        pythonCmd += "        if mc.objectType(material) == 'shadingEngine':\n";
        pythonCmd += "            shading_engines = [material]\n";
        pythonCmd += "        else:\n";
        pythonCmd += "            connections = mc.listConnections(material, type='shadingEngine', destination=True) or []\n";
        pythonCmd += "            shading_engines = list(set(connections))\n";
        pythonCmd += "        for sg in shading_engines:\n";
        pythonCmd += "            try:\n";
        pythonCmd += "                members = mc.sets(sg, query=True) or []\n";
        pythonCmd += "                for member in members:\n";
        pythonCmd += "                    if mc.objectType(member, isAType='shape'):\n";
        pythonCmd += "                        transforms = mc.listRelatives(member, parent=True, fullPath=True) or []\n";
        pythonCmd += "                        objects.extend(transforms)\n";
        pythonCmd += "                    elif mc.objectType(member, isAType='transform'):\n";
        pythonCmd += "                        objects.append(member)\n";
        pythonCmd += "            except Exception as e:\n";
        pythonCmd += "                print('Error processing shading engine: ' + str(e))\n";
        pythonCmd += "    # Select objects and expand in outliner\n";
        pythonCmd += "    if objects:\n";
        pythonCmd += "        mc.select(objects, replace=True)\n";
        pythonCmd += "        outliner_panels = mc.getPanel(type='outlinerPanel')\n";
        pythonCmd += "        if outliner_panels:\n";
        pythonCmd += "            for panel in outliner_panels:\n";
        pythonCmd += "                outliner = mc.outlinerPanel(panel, query=True, outlinerEditor=True)\n";
        pythonCmd += "                if outliner:\n";
        pythonCmd += "                    mc.outlinerEditor(outliner, edit=True, showSelected=True)\n";
        pythonCmd += "    else:\n";
        pythonCmd += "        mc.select(materials, replace=True)\n";
        pythonCmd += "except Exception as e:\n";
        pythonCmd += "    print('Error selecting objects for materials: ' + str(e))\n";

        MGlobal::executePythonCommand(pythonCmd);
    } else {
        // Mixed selection or all regular nodes - select them directly
        MSelectionList selList;
        MStatus status;

        for (const QString& nodeName : nodeNames) {
            status = selList.add(nodeName.toUtf8().constData());
            if (status != MS::kSuccess) {
                MGlobal::displayWarning("Failed to add node to selection: " + MString(nodeName.toUtf8().constData()));
            }
        }

        if (selList.length() > 0) {
            MGlobal::setActiveSelectionList(selList);

            // Expand outliner to show selected nodes
            MString pythonCmd =
                "import maya.cmds as mc\n"
                "outliner_panels = mc.getPanel(type='outlinerPanel')\n"
                "if outliner_panels:\n"
                "    for panel in outliner_panels:\n"
                "        outliner = mc.outlinerPanel(panel, query=True, outlinerEditor=True)\n"
                "        if outliner:\n"
                "            mc.outlinerEditor(outliner, edit=True, showSelected=True)\n";
            MGlobal::executePythonCommand(pythonCmd);
        }
    }
}

QStringList ExtraAttrUI::getEnumFieldNames(const QString& nodeName, const QString& attrName) const
{
    QStringList result;

    MStatus status;
    MSelectionList selList;
    status = selList.add(nodeName.toUtf8().constData());
    if (status != MS::kSuccess) {
        return result;
    }

    MObject nodeObj;
    status = selList.getDependNode(0, nodeObj);
    if (status != MS::kSuccess || nodeObj.isNull()) {
        return result;
    }

    MFnDependencyNode fnDep(nodeObj, &status);
    if (status != MS::kSuccess) {
        return result;
    }

    MObject attr = fnDep.attribute(attrName.toUtf8().constData(), &status);
    if (status != MS::kSuccess || attr.isNull()) {
        return result;
    }

    if (attr.apiType() != MFn::kEnumAttribute) {
        return result;
    }

    MFnEnumAttribute fnEnum(attr, &status);
    if (status != MS::kSuccess) {
        return result;
    }

    short minValue, maxValue;
    status = fnEnum.getMin(minValue);
    if (status != MS::kSuccess) {
        minValue = 0;
    }

    status = fnEnum.getMax(maxValue);
    if (status != MS::kSuccess) {
        maxValue = 0;
    }

    for (short i = minValue; i <= maxValue; ++i) {
        MString fieldName = fnEnum.fieldName(i, &status);
        if (status == MS::kSuccess && fieldName.length() > 0) {
            result.append(QString::fromUtf8(fieldName.asChar()));
        }
    }

    return result;
}

