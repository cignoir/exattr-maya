#include "ExtraAttrManagerCmd.h"
#include "ExtraAttrUI.h"
#include "ExtraAttrScanner.h"
#include "MayaUtils.h"
#include <maya/MArgDatabase.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MFnEnumAttribute.h>
#include <maya/MGlobal.h>
#include <maya/MPlug.h>
#include <maya/MFnNumericData.h>

const char* ExtraAttrManagerCmd::commandName = "exAttrEditor";

// Command flag definitions
const char* ExtraAttrManagerCmd::kScanFlag = "-s";
const char* ExtraAttrManagerCmd::kScanFlagLong = "-scan";
const char* ExtraAttrManagerCmd::kListFlag = "-ls";
const char* ExtraAttrManagerCmd::kListFlagLong = "-list";
const char* ExtraAttrManagerCmd::kNodesFlag = "-n";
const char* ExtraAttrManagerCmd::kNodesFlagLong = "-nodes";
const char* ExtraAttrManagerCmd::kUIFlag = "-ui";
const char* ExtraAttrManagerCmd::kUIFlagLong = "-showUI";
const char* ExtraAttrManagerCmd::kCloseUIFlag = "-cui";
const char* ExtraAttrManagerCmd::kCloseUIFlagLong = "-closeUI";
const char* ExtraAttrManagerCmd::kEditFlag = "-e";
const char* ExtraAttrManagerCmd::kEditFlagLong = "-edit";
const char* ExtraAttrManagerCmd::kDeleteFlag = "-d";
const char* ExtraAttrManagerCmd::kDeleteFlagLong = "-delete";
const char* ExtraAttrManagerCmd::kAddFlag = "-a";
const char* ExtraAttrManagerCmd::kAddFlagLong = "-add";
const char* ExtraAttrManagerCmd::kHelpFlag = "-h";
const char* ExtraAttrManagerCmd::kHelpFlagLong = "-help";

ExtraAttrManagerCmd::ExtraAttrManagerCmd()
    : m_isUndoable(false)
{
}

ExtraAttrManagerCmd::~ExtraAttrManagerCmd()
{
}

void* ExtraAttrManagerCmd::creator()
{
    return new ExtraAttrManagerCmd();
}

MSyntax ExtraAttrManagerCmd::newSyntax()
{
    MSyntax syntax;

    syntax.addFlag(kScanFlag, kScanFlagLong);
    syntax.addFlag(kListFlag, kListFlagLong);
    syntax.addFlag(kNodesFlag, kNodesFlagLong, MSyntax::kString);
    syntax.addFlag(kUIFlag, kUIFlagLong);
    syntax.addFlag(kCloseUIFlag, kCloseUIFlagLong);
    syntax.addFlag(kEditFlag, kEditFlagLong, MSyntax::kString, MSyntax::kString, MSyntax::kString);
    syntax.addFlag(kDeleteFlag, kDeleteFlagLong, MSyntax::kString, MSyntax::kString);
    syntax.addFlag(kAddFlag, kAddFlagLong, MSyntax::kString, MSyntax::kString, MSyntax::kString);
    syntax.addFlag(kHelpFlag, kHelpFlagLong);

    return syntax;
}

MStatus ExtraAttrManagerCmd::doIt(const MArgList& args)
{
    MStatus status;
    MArgDatabase argData(syntax(), args, &status);

    if (status != MS::kSuccess) {
        MGlobal::displayError("Error parsing arguments");
        return status;
    }

    // Check help flag
    if (argData.isFlagSet(kHelpFlag)) {
        return doHelp();
    }

    // Scan flag
    if (argData.isFlagSet(kScanFlag)) {
        return doScan();
    }

    // List flag
    if (argData.isFlagSet(kListFlag)) {
        return doList();
    }

    // Node list flag
    if (argData.isFlagSet(kNodesFlag)) {
        MString attrName;
        status = argData.getFlagArgument(kNodesFlag, 0, attrName);
        if (status != MS::kSuccess) {
            MGlobal::displayError("Invalid argument for -nodes flag");
            return status;
        }
        return doGetNodes(attrName);
    }

    // UI flag
    if (argData.isFlagSet(kUIFlag)) {
        return doShowUI();
    }

    // Close UI flag
    if (argData.isFlagSet(kCloseUIFlag)) {
        return doCloseUI();
    }

    // Edit flag
    if (argData.isFlagSet(kEditFlag)) {
        MString nodeName, attrName, value;
        status = argData.getFlagArgument(kEditFlag, 0, nodeName);
        if (status != MS::kSuccess) {
            MGlobal::displayError("Invalid node name for -edit flag");
            return status;
        }
        status = argData.getFlagArgument(kEditFlag, 1, attrName);
        if (status != MS::kSuccess) {
            MGlobal::displayError("Invalid attribute name for -edit flag");
            return status;
        }
        status = argData.getFlagArgument(kEditFlag, 2, value);
        if (status != MS::kSuccess) {
            MGlobal::displayError("Invalid value for -edit flag");
            return status;
        }
        return doEdit(nodeName, attrName, value);
    }

    // Delete flag
    if (argData.isFlagSet(kDeleteFlag)) {
        MString nodeName, attrName;
        status = argData.getFlagArgument(kDeleteFlag, 0, nodeName);
        if (status != MS::kSuccess) {
            MGlobal::displayError("Invalid node name for -delete flag");
            return status;
        }
        status = argData.getFlagArgument(kDeleteFlag, 1, attrName);
        if (status != MS::kSuccess) {
            MGlobal::displayError("Invalid attribute name for -delete flag");
            return status;
        }
        return doDelete(nodeName, attrName);
    }

    // Add flag
    if (argData.isFlagSet(kAddFlag)) {
        MString nodeName, attrName, attrType;
        status = argData.getFlagArgument(kAddFlag, 0, nodeName);
        if (status != MS::kSuccess) {
            MGlobal::displayError("Invalid node name for -add flag");
            return status;
        }
        status = argData.getFlagArgument(kAddFlag, 1, attrName);
        if (status != MS::kSuccess) {
            MGlobal::displayError("Invalid attribute name for -add flag");
            return status;
        }
        status = argData.getFlagArgument(kAddFlag, 2, attrType);
        if (status != MS::kSuccess) {
            MGlobal::displayError("Invalid attribute type for -add flag");
            return status;
        }
        return doAdd(nodeName, attrName, attrType);
    }

    // Show UI if no flag is specified
    return doShowUI();
}

MStatus ExtraAttrManagerCmd::redoIt()
{
    // Currently few undoable operations, so basically do nothing
    return MS::kSuccess;
}

MStatus ExtraAttrManagerCmd::undoIt()
{
    // Undo processing (extensible in the future)
    return MS::kSuccess;
}

bool ExtraAttrManagerCmd::isUndoable() const
{
    return m_isUndoable;
}

MStatus ExtraAttrManagerCmd::doScan()
{
    ExtraAttrScanner scanner;
    if (!scanner.scanScene()) {
        MGlobal::displayError("Failed to scan scene");
        return MS::kFailure;
    }

    int totalAttrs, totalNodes;
    scanner.getStatistics(totalAttrs, totalNodes);

    setResult(MString("Scan complete: Found ") + totalAttrs + " extra attributes in " + totalNodes + " nodes");
    return MS::kSuccess;
}

MStatus ExtraAttrManagerCmd::doList()
{
    ExtraAttrScanner scanner;
    if (!scanner.scanScene()) {
        MGlobal::displayError("Failed to scan scene");
        return MS::kFailure;
    }

    const auto& attrMap = scanner.getAttributeInfoMap();

    MStringArray result;
    for (const auto& pair : attrMap) {
        MString line = pair.first + " (" + pair.second->typeName + ") - " + pair.second->usageCount + " nodes";
        result.append(line);
    }

    setResult(result);
    return MS::kSuccess;
}

MStatus ExtraAttrManagerCmd::doGetNodes(const MString& attrName)
{
    ExtraAttrScanner scanner;
    if (!scanner.scanScene()) {
        MGlobal::displayError("Failed to scan scene");
        return MS::kFailure;
    }

    std::vector<NodeAttributeValue> nodeValues = scanner.getNodesWithAttribute(attrName);

    MStringArray result;
    for (const auto& nodeValue : nodeValues) {
        MString line = nodeValue.nodeName + " (" + nodeValue.nodeType + ") = " + nodeValue.valueStr;
        result.append(line);
    }

    setResult(result);
    return MS::kSuccess;
}

MStatus ExtraAttrManagerCmd::doShowUI()
{
    ExtraAttrUI* ui = ExtraAttrUI::instance();
    if (!ui) {
        MGlobal::displayError("Failed to create UI instance");
        return MS::kFailure;
    }

    ui->showUI();
    return MS::kSuccess;
}

MStatus ExtraAttrManagerCmd::doCloseUI()
{
    ExtraAttrUI* ui = ExtraAttrUI::instance();
    if (ui) {
        ui->closeUI();
    }
    return MS::kSuccess;
}

MStatus ExtraAttrManagerCmd::doEdit(const MString& nodeName, const MString& attrName, const MString& value)
{
    MStatus status;
    MFnDependencyNode fnDep;

    // Get node
    if (!MayaUtils::getDependencyNodeFromName(nodeName, fnDep)) {
        MGlobal::displayError(MString("Node not found: ") + nodeName);
        return MS::kFailure;
    }

    // Get attribute
    MObject attr = fnDep.attribute(attrName, &status);
    if (status != MS::kSuccess || attr.isNull()) {
        MGlobal::displayError(MString("Attribute not found: ") + attrName);
        return MS::kFailure;
    }

    MPlug plug = fnDep.findPlug(attr, &status);
    if (status != MS::kSuccess) {
        MGlobal::displayError("Failed to find plug");
        return status;
    }

    // Set value using utility function
    if (!MayaUtils::setAttributeValueFromString(plug, attr, value)) {
        MGlobal::displayError("Failed to set attribute value");
        return MS::kFailure;
    }

    m_isUndoable = true;
    MGlobal::displayInfo(MString("Set ") + nodeName + "." + attrName + " = " + value);
    return MS::kSuccess;
}

MStatus ExtraAttrManagerCmd::doDelete(const MString& nodeName, const MString& attrName)
{
    MStatus status;
    MFnDependencyNode fnDep;

    // Get node
    if (!MayaUtils::getDependencyNodeFromName(nodeName, fnDep)) {
        MGlobal::displayError(MString("Node not found: ") + nodeName);
        return MS::kFailure;
    }

    // Get attribute
    MObject attr = fnDep.attribute(attrName, &status);
    if (status != MS::kSuccess || attr.isNull()) {
        MGlobal::displayError(MString("Attribute not found: ") + attrName);
        return MS::kFailure;
    }

    // Delete attribute
    status = fnDep.removeAttribute(attr);
    if (status != MS::kSuccess) {
        MGlobal::displayError("Failed to remove attribute");
        return status;
    }

    m_isUndoable = true;
    MGlobal::displayInfo(MString("Deleted attribute: ") + nodeName + "." + attrName);
    return MS::kSuccess;
}

MStatus ExtraAttrManagerCmd::doAdd(const MString& nodeName, const MString& attrName, const MString& attrType)
{
    MStatus status;
    MFnDependencyNode fnDep;

    // Get node
    if (!MayaUtils::getDependencyNodeFromName(nodeName, fnDep)) {
        MGlobal::displayError(MString("Node not found: ") + nodeName);
        return MS::kFailure;
    }

    // Create attribute according to attribute type
    MObject attr;

    if (attrType == "double" || attrType == "float") {
        MFnNumericAttribute nAttr;
        attr = nAttr.create(attrName, attrName, MFnNumericData::kDouble, 0.0, &status);
        if (status != MS::kSuccess) {
            MGlobal::displayError("Failed to create numeric attribute");
            return status;
        }
        nAttr.setKeyable(true);
    } else if (attrType == "int" || attrType == "long") {
        MFnNumericAttribute nAttr;
        attr = nAttr.create(attrName, attrName, MFnNumericData::kInt, 0, &status);
        if (status != MS::kSuccess) {
            MGlobal::displayError("Failed to create numeric attribute");
            return status;
        }
        nAttr.setKeyable(true);
    } else if (attrType == "bool" || attrType == "boolean") {
        MFnNumericAttribute nAttr;
        attr = nAttr.create(attrName, attrName, MFnNumericData::kBoolean, false, &status);
        if (status != MS::kSuccess) {
            MGlobal::displayError("Failed to create numeric attribute");
            return status;
        }
        nAttr.setKeyable(true);
    } else if (attrType == "string") {
        MFnTypedAttribute tAttr;
        attr = tAttr.create(attrName, attrName, MFnData::kString, &status);
        if (status != MS::kSuccess) {
            MGlobal::displayError("Failed to create typed attribute");
            return status;
        }
    } else if (attrType == "enum") {
        MFnEnumAttribute eAttr;
        attr = eAttr.create(attrName, attrName, 0, &status);
        if (status != MS::kSuccess) {
            MGlobal::displayError("Failed to create enum attribute");
            return status;
        }
        eAttr.addField("option1", 0);
        eAttr.addField("option2", 1);
    } else {
        MGlobal::displayError(MString("Unsupported attribute type: ") + attrType);
        return MS::kFailure;
    }

    // Add attribute to node
    status = fnDep.addAttribute(attr);
    if (status != MS::kSuccess) {
        MGlobal::displayError("Failed to add attribute to node");
        return status;
    }

    m_isUndoable = true;
    MGlobal::displayInfo(MString("Added attribute: ") + nodeName + "." + attrName + " (" + attrType + ")");
    return MS::kSuccess;
}

MStatus ExtraAttrManagerCmd::doHelp()
{
    MString helpText =
        "exAttrManager - Extra Attribute Manager Command\n"
        "\n"
        "Usage:\n"
        "  exAttrManager [flags]\n"
        "\n"
        "Flags:\n"
        "  -scan/-s                         : Scan scene for extra attributes\n"
        "  -list/-ls                        : List all extra attributes\n"
        "  -nodes/-n <attrName>             : Get nodes with specific attribute\n"
        "  -ui/-showUI                      : Show UI window\n"
        "  -closeUI/-cui                    : Close UI window\n"
        "  -edit/-e <node> <attr> <value>   : Edit attribute value\n"
        "  -delete/-d <node> <attr>         : Delete attribute\n"
        "  -add/-a <node> <attr> <type>     : Add new attribute\n"
        "  -help/-h                         : Show this help\n"
        "\n"
        "Examples:\n"
        "  exAttrManager -scan;\n"
        "  exAttrManager -list;\n"
        "  exAttrManager -nodes \"myCustomAttr\";\n"
        "  exAttrManager -ui;\n"
        "  exAttrManager -edit \"pCube1\" \"myAttr\" \"100\";\n"
        "  exAttrManager -delete \"pCube1\" \"oldAttr\";\n"
        "  exAttrManager -add \"pCube1\" \"newAttr\" \"double\";\n";

    MGlobal::displayInfo(helpText);
    return MS::kSuccess;
}
