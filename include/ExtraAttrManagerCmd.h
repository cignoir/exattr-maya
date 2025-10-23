#ifndef EXTRA_ATTR_MANAGER_CMD_H
#define EXTRA_ATTR_MANAGER_CMD_H

#include <maya/MPxCommand.h>
#include <maya/MArgList.h>
#include <maya/MArgDatabase.h>
#include <maya/MString.h>
#include <maya/MSyntax.h>

/**
 * @class ExtraAttrManagerCmd
 * @brief MEL command for Extra Attribute management
 *
 * Command name: exAttrEditor
 *
 * Flags:
 *   -scan/-s         : Scan the scene to detect Extra Attributes
 *   -list/-ls        : Get a list of detected Extra Attributes
 *   -nodes/-n <attr> : Get a list of nodes with the specified attribute
 *   -ui/-ui          : Open UI
 *   -closeUI/-cui    : Close UI
 *   -edit/-e <node> <attr> <value> : Edit attribute value
 *   -delete/-d <node> <attr> : Delete attribute
 *   -add/-a <node> <attr> <type> : Add attribute
 *   -help/-h         : Show help
 *
 * Usage examples:
 *   exAttrEditor -scan;
 *   exAttrEditor -list;
 *   exAttrEditor -nodes "myCustomAttr";
 *   exAttrEditor -ui;
 *   exAttrEditor -edit "pCube1" "myAttr" "100";
 *   exAttrEditor -delete "pCube1" "oldAttr";
 *   exAttrEditor -add "pCube1" "newAttr" "double";
 */
class ExtraAttrManagerCmd : public MPxCommand {
public:
    ExtraAttrManagerCmd();
    ~ExtraAttrManagerCmd() override;

    MStatus doIt(const MArgList& args) override;
    MStatus redoIt() override;
    MStatus undoIt() override;
    bool isUndoable() const override;

    static void* creator();
    static MSyntax newSyntax();

    static const char* commandName;

private:
    /**
     * @brief Scan the scene to detect Extra Attributes
     */
    MStatus doScan();

    /**
     * @brief Get a list of Extra Attributes
     */
    MStatus doList();

    /**
     * @brief Get a list of nodes with the specified attribute
     * @param attrName Attribute name
     */
    MStatus doGetNodes(const MString& attrName);

    /**
     * @brief Open UI
     */
    MStatus doShowUI();

    /**
     * @brief Close UI
     */
    MStatus doCloseUI();

    /**
     * @brief Edit attribute value
     * @param nodeName Node name
     * @param attrName Attribute name
     * @param value New value
     */
    MStatus doEdit(const MString& nodeName, const MString& attrName, const MString& value);

    /**
     * @brief Delete attribute
     * @param nodeName Node name
     * @param attrName Attribute name
     */
    MStatus doDelete(const MString& nodeName, const MString& attrName);

    /**
     * @brief Add attribute
     * @param nodeName Node name
     * @param attrName Attribute name
     * @param attrType Attribute type
     */
    MStatus doAdd(const MString& nodeName, const MString& attrName, const MString& attrType);

    /**
     * @brief Show help
     */
    MStatus doHelp();

private:
    // Command flags
    static const char* kScanFlag;
    static const char* kScanFlagLong;
    static const char* kListFlag;
    static const char* kListFlagLong;
    static const char* kNodesFlag;
    static const char* kNodesFlagLong;
    static const char* kUIFlag;
    static const char* kUIFlagLong;
    static const char* kCloseUIFlag;
    static const char* kCloseUIFlagLong;
    static const char* kEditFlag;
    static const char* kEditFlagLong;
    static const char* kDeleteFlag;
    static const char* kDeleteFlagLong;
    static const char* kAddFlag;
    static const char* kAddFlagLong;
    static const char* kHelpFlag;
    static const char* kHelpFlagLong;

    // Undo data
    struct UndoData {
        MString nodeName;
        MString attrName;
        MString oldValue;
        bool wasDeleted;
        bool wasAdded;

        UndoData() : nodeName(""), attrName(""), oldValue(""),
                     wasDeleted(false), wasAdded(false) {}
    };

    UndoData m_undoData;
    bool m_isUndoable;
};

#endif // EXTRA_ATTR_MANAGER_CMD_H
