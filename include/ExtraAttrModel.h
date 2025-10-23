#ifndef EXTRA_ATTR_MODEL_H
#define EXTRA_ATTR_MODEL_H

#include <QAbstractItemModel>
#include <QList>
#include <QString>
#include <memory>
#include <maya/MString.h>
#include "ExtraAttrScanner.h"

/**
 * @class ExtraAttrModel
 * @brief Class to manage Extra Attribute list using Qt Model/View architecture
 */
class ExtraAttrModel : public QAbstractTableModel {
    Q_OBJECT

public:
    enum Column {
        COL_ATTR_NAME = 0,      // Attribute name
        COL_TYPE,               // Type
        COL_USAGE_COUNT,        // Number of nodes using this attribute
        COL_IS_ARRAY,           // Is array attribute
        COL_COUNT
    };

    explicit ExtraAttrModel(QObject* parent = nullptr);
    ~ExtraAttrModel();

    // QAbstractTableModel overrides
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    /**
     * @brief Load data from scanner
     * @param scanner Extra Attribute scanner
     */
    void loadFromScanner(const ExtraAttrScanner& scanner);

    /**
     * @brief Clear model
     */
    void clear();

    /**
     * @brief Get attribute name at specified row
     * @param row Row number
     * @return Attribute name
     */
    QString getAttributeName(int row) const;

    /**
     * @brief Get attribute info at specified row
     * @param row Row number
     * @return Pointer to attribute info
     */
    std::shared_ptr<AttributeInfo> getAttributeInfo(int row) const;

    /**
     * @brief Sort function
     * @param column Column number
     * @param order Sort order
     */
    void sort(int column, Qt::SortOrder order = Qt::AscendingOrder) override;

private:
    struct AttributeRow {
        QString name;
        QString typeName;
        int usageCount;
        bool isArray;
        std::shared_ptr<AttributeInfo> info;
    };

    QList<AttributeRow> m_rows;
};

/**
 * @class NodeAttributeModel
 * @brief Model to display nodes with a specific Extra Attribute and their values
 */
class NodeAttributeModel : public QAbstractTableModel {
    Q_OBJECT

public:
    enum Column {
        COL_NODE_NAME = 0,      // Node name
        COL_NODE_TYPE,          // Node type
        COL_VALUE,              // Attribute value
        COL_COUNT
    };

    explicit NodeAttributeModel(QObject* parent = nullptr);
    ~NodeAttributeModel();

    // QAbstractTableModel overrides
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;
    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;

    /**
     * @brief Set list of nodes and attribute values
     * @param attrName Attribute name
     * @param nodeValues List of nodes and attribute values
     */
    void setNodeValues(const QString& attrName, const std::vector<NodeAttributeValue>& nodeValues);

    /**
     * @brief Clear model
     */
    void clear();

    /**
     * @brief Get node name at specified row
     * @param row Row number
     * @return Node name
     */
    QString getNodeName(int row) const;

    /**
     * @brief Get currently displayed attribute name
     * @return Attribute name
     */
    QString getCurrentAttributeName() const;

    /**
     * @brief Sort table
     * @param column Column to sort
     * @param order Sort order
     */
    void sort(int column, Qt::SortOrder order = Qt::AscendingOrder) override;

signals:
    /**
     * @brief Emitted when attribute value is edited
     * @param nodeName Node name
     * @param attrName Attribute name
     * @param newValue New value
     */
    void valueChanged(const QString& nodeName, const QString& attrName, const QString& newValue);

private:
    struct NodeRow {
        QString nodeName;
        QString nodeType;
        QString value;
    };

    QString m_currentAttrName;
    QList<NodeRow> m_rows;
};

#endif // EXTRA_ATTR_MODEL_H
