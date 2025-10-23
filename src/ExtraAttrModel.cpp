#include "ExtraAttrModel.h"
#include <algorithm>

// ========== ExtraAttrModel ==========

ExtraAttrModel::ExtraAttrModel(QObject* parent)
    : QAbstractTableModel(parent)
{
}

ExtraAttrModel::~ExtraAttrModel()
{
}

int ExtraAttrModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return m_rows.size();
}

int ExtraAttrModel::columnCount(const QModelIndex& parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return COL_COUNT;
}

QVariant ExtraAttrModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() >= m_rows.size()) {
        return QVariant();
    }

    const AttributeRow& row = m_rows[index.row()];

    if (role == Qt::DisplayRole) {
        switch (index.column()) {
            case COL_ATTR_NAME:
                return row.name;
            case COL_TYPE:
                return row.typeName;
            case COL_USAGE_COUNT:
                return row.usageCount;
            case COL_IS_ARRAY:
                return row.isArray ? "Yes" : "No";
            default:
                return QVariant();
        }
    } else if (role == Qt::TextAlignmentRole) {
        if (index.column() == COL_USAGE_COUNT) {
            return Qt::AlignCenter;
        }
    }

    return QVariant();
}

QVariant ExtraAttrModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole) {
        return QVariant();
    }

    if (orientation == Qt::Horizontal) {
        switch (section) {
            case COL_ATTR_NAME:
                return "Name";
            case COL_TYPE:
                return "Type";
            case COL_USAGE_COUNT:
                return "Usage Count";
            case COL_IS_ARRAY:
                return "Array";
            default:
                return QVariant();
        }
    }

    return QVariant();
}

void ExtraAttrModel::loadFromScanner(const ExtraAttrScanner& scanner)
{
    beginResetModel();

    m_rows.clear();

    const auto& attrMap = scanner.getAttributeInfoMap();
    for (const auto& pair : attrMap) {
        AttributeRow row;
        row.name = QString::fromUtf8(pair.first.asChar());
        row.typeName = QString::fromUtf8(pair.second->typeName.asChar());
        row.usageCount = pair.second->usageCount;
        row.isArray = pair.second->isArray;
        row.info = pair.second;

        m_rows.append(row);
    }

    endResetModel();
}

void ExtraAttrModel::clear()
{
    beginResetModel();
    m_rows.clear();
    endResetModel();
}

QString ExtraAttrModel::getAttributeName(int row) const
{
    if (row < 0 || row >= m_rows.size()) {
        return QString();
    }
    return m_rows[row].name;
}

std::shared_ptr<AttributeInfo> ExtraAttrModel::getAttributeInfo(int row) const
{
    if (row < 0 || row >= m_rows.size()) {
        return nullptr;
    }
    return m_rows[row].info;
}

void ExtraAttrModel::sort(int column, Qt::SortOrder order)
{
    if (m_rows.isEmpty()) {
        return;
    }

    emit layoutAboutToBeChanged();

    std::sort(m_rows.begin(), m_rows.end(), [column, order](const AttributeRow& a, const AttributeRow& b) {
        bool result = false;

        switch (column) {
            case COL_ATTR_NAME:
                result = a.name < b.name;
                break;
            case COL_TYPE:
                result = a.typeName < b.typeName;
                break;
            case COL_USAGE_COUNT:
                result = a.usageCount < b.usageCount;
                break;
            case COL_IS_ARRAY:
                result = a.isArray < b.isArray;
                break;
            default:
                result = a.name < b.name;
                break;
        }

        return (order == Qt::AscendingOrder) ? result : !result;
    });

    emit layoutChanged();
}

// ========== NodeAttributeModel ==========

NodeAttributeModel::NodeAttributeModel(QObject* parent)
    : QAbstractTableModel(parent)
{
}

NodeAttributeModel::~NodeAttributeModel()
{
}

int NodeAttributeModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return m_rows.size();
}

int NodeAttributeModel::columnCount(const QModelIndex& parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return COL_COUNT;
}

QVariant NodeAttributeModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() >= m_rows.size()) {
        return QVariant();
    }

    const NodeRow& row = m_rows[index.row()];

    if (role == Qt::DisplayRole || role == Qt::EditRole) {
        switch (index.column()) {
            case COL_NODE_NAME:
                return row.nodeName;
            case COL_NODE_TYPE:
                return row.nodeType;
            case COL_VALUE:
                return row.value;
            default:
                return QVariant();
        }
    }

    return QVariant();
}

QVariant NodeAttributeModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole) {
        return QVariant();
    }

    if (orientation == Qt::Horizontal) {
        switch (section) {
            case COL_NODE_NAME:
                return "Node Name";
            case COL_NODE_TYPE:
                return "Node Type";
            case COL_VALUE:
                return "Value";
            default:
                return QVariant();
        }
    }

    return QVariant();
}

Qt::ItemFlags NodeAttributeModel::flags(const QModelIndex& index) const
{
    if (!index.isValid()) {
        return Qt::NoItemFlags;
    }

    // Only Value column is editable
    if (index.column() == COL_VALUE) {
        return QAbstractTableModel::flags(index) | Qt::ItemIsEditable;
    }

    return QAbstractTableModel::flags(index);
}

bool NodeAttributeModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if (!index.isValid() || index.row() >= m_rows.size() || role != Qt::EditRole) {
        return false;
    }

    if (index.column() != COL_VALUE) {
        return false;
    }

    // Update data
    m_rows[index.row()].value = value.toString();

    // Emit signal to trigger Maya API update
    emit valueChanged(m_rows[index.row()].nodeName, m_currentAttrName, value.toString());
    emit dataChanged(index, index);

    return true;
}

void NodeAttributeModel::setNodeValues(const QString& attrName, const std::vector<NodeAttributeValue>& nodeValues)
{
    beginResetModel();

    m_currentAttrName = attrName;
    m_rows.clear();

    for (const auto& nodeValue : nodeValues) {
        NodeRow row;
        row.nodeName = QString::fromUtf8(nodeValue.nodeName.asChar());
        row.nodeType = QString::fromUtf8(nodeValue.nodeType.asChar());
        row.value = QString::fromUtf8(nodeValue.valueStr.asChar());

        m_rows.append(row);
    }

    endResetModel();
}

void NodeAttributeModel::clear()
{
    beginResetModel();
    m_currentAttrName.clear();
    m_rows.clear();
    endResetModel();
}

QString NodeAttributeModel::getNodeName(int row) const
{
    if (row < 0 || row >= m_rows.size()) {
        return QString();
    }
    return m_rows[row].nodeName;
}

QString NodeAttributeModel::getCurrentAttributeName() const
{
    return m_currentAttrName;
}

void NodeAttributeModel::sort(int column, Qt::SortOrder order)
{
    if (m_rows.isEmpty()) {
        return;
    }

    emit layoutAboutToBeChanged();

    std::sort(m_rows.begin(), m_rows.end(), [column, order](const NodeRow& a, const NodeRow& b) {
        bool result = false;

        switch (column) {
            case COL_NODE_NAME:
                result = a.nodeName < b.nodeName;
                break;
            case COL_NODE_TYPE:
                result = a.nodeType < b.nodeType;
                break;
            case COL_VALUE:
                result = a.value < b.value;
                break;
            default:
                result = a.nodeName < b.nodeName;
                break;
        }

        return (order == Qt::AscendingOrder) ? result : !result;
    });

    emit layoutChanged();
}
