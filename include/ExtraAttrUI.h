#ifndef EXTRA_ATTR_UI_H
#define EXTRA_ATTR_UI_H

#include <QMainWindow>
#include <QTableView>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QSplitter>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMenu>
#include <QStyledItemDelegate>
#include <QComboBox>
#include <QRadioButton>
#include <QButtonGroup>
#include <QSortFilterProxyModel>
#include <memory>

#include "ExtraAttrModel.h"
#include "ExtraAttrScanner.h"

// Forward declaration
class ExtraAttrUI;

/**
 * @class EnumAttributeDelegate
 * @brief Custom ItemDelegate for Enum attributes (dropdown editing)
 */
class EnumAttributeDelegate : public QStyledItemDelegate {
    Q_OBJECT

public:
    explicit EnumAttributeDelegate(ExtraAttrUI* parent);

    QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    void setEditorData(QWidget* editor, const QModelIndex& index) const override;
    void setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const override;
    void updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index) const override;

private:
    ExtraAttrUI* m_ui;
};

/**
 * @class ExtraAttrUI
 * @brief Main UI window for Extra Attribute management
 *
 * Layout:
 *   Top: Toolbar (scan button, search bar, statistics display)
 *   Left: Extra Attribute list table
 *   Right: List table of nodes with selected attribute and their values
 */
class ExtraAttrUI : public QMainWindow {
    Q_OBJECT

public:
    explicit ExtraAttrUI(QWidget* parent = nullptr);
    ~ExtraAttrUI();

    /**
     * @brief Get singleton instance
     */
    static ExtraAttrUI* instance();

    /**
     * @brief Destroy singleton instance (call on plugin unload)
     */
    static void destroyInstance();

    /**
     * @brief Show UI
     */
    void showUI();

    /**
     * @brief Close UI
     */
    void closeUI();

    /**
     * @brief Get enum attribute option list (public for access from EnumAttributeDelegate)
     */
    QStringList getEnumFieldNames(const QString& nodeName, const QString& attrName) const;

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    /**
     * @brief Handler when scan button is clicked
     */
    void onScanButtonClicked();

    /**
     * @brief Handler when attribute table selection is changed
     */
    void onAttributeSelectionChanged(const QModelIndex& current, const QModelIndex& previous);

    /**
     * @brief Handler when search text is changed
     */
    void onSearchTextChanged(const QString& text);

    /**
     * @brief Handler when node table value is changed
     */
    void onNodeValueChanged(const QString& nodeName, const QString& attrName, const QString& newValue);

    /**
     * @brief Context menu for attribute table
     */
    void onAttributeContextMenu(const QPoint& pos);

    /**
     * @brief Context menu for node table
     */
    void onNodeContextMenu(const QPoint& pos);

    /**
     * @brief Delete attribute action
     */
    void onDeleteAttribute();

    /**
     * @brief Add attribute action
     */
    void onAddAttribute();

    /**
     * @brief Batch edit action
     */
    void onBatchEdit();

    /**
     * @brief Select node action
     */
    void onSelectNode();

    /**
     * @brief Select polygons assigned to material
     */
    void onSelectAssignedPolygons();

    /**
     * @brief Handler when attribute filter is changed
     */
    void onAttributeFilterChanged(const QString& text);

    /**
     * @brief Handler when node filter is changed
     */
    void onNodeFilterChanged(const QString& text);

    /**
     * @brief Handler when node table selection is changed
     */
    void onNodeSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected);

private:
    /**
     * @brief Initialize UI
     */
    void setupUI();

    /**
     * @brief Create toolbar
     */
    QWidget* createToolBar();

    /**
     * @brief Update statistics
     */
    void updateStatistics();

    /**
     * @brief Set attribute value using Maya API
     */
    bool setAttributeValue(const QString& nodeName, const QString& attrName, const QString& value);

    /**
     * @brief Delete attribute using Maya API
     */
    bool deleteAttribute(const QString& nodeName, const QString& attrName);

    /**
     * @brief Add attribute using Maya API
     */
    bool addAttribute(const QString& nodeName, const QString& attrName, const QString& attrType);

    /**
     * @brief Select node in Maya
     */
    bool selectNodeInMaya(const QString& nodeName);

    /**
     * @brief Check if node is a shading node (material)
     */
    bool isShadingNode(const QString& nodeName);

    /**
     * @brief Select polygons assigned to material
     */
    bool selectPolygonsWithMaterial(const QString& materialName);

private:
    // Singleton instance
    static ExtraAttrUI* s_instance;

    // UI elements
    QTableView* m_attributeTableView;       // Extra Attribute list
    QTableView* m_nodeTableView;            // Node list
    QLineEdit* m_attributeFilterLineEdit;   // Attribute filter
    QLineEdit* m_nodeFilterLineEdit;        // Node filter
    QRadioButton* m_filterNodeNameRadio;    // Filter by node name
    QRadioButton* m_filterNodeTypeRadio;    // Filter by node type
    QRadioButton* m_filterValueRadio;       // Filter by value
    QButtonGroup* m_filterButtonGroup;      // Radio button group
    QPushButton* m_scanButton;              // Scan button
    QLabel* m_statsLabel;                   // Statistics label

    // Data models
    ExtraAttrModel* m_attributeModel;
    NodeAttributeModel* m_nodeModel;
    QSortFilterProxyModel* m_attributeProxyModel;
    QSortFilterProxyModel* m_nodeProxyModel;

    // Scanner
    std::unique_ptr<ExtraAttrScanner> m_scanner;

    // Currently selected attribute name
    QString m_currentAttributeName;

    // Original data for filtering
    QList<int> m_filteredRows;
};

#endif // EXTRA_ATTR_UI_H
