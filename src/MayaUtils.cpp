#include "MayaUtils.h"
#include <maya/MSelectionList.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MFnEnumAttribute.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MFnNumericData.h>
#include <maya/MGlobal.h>

namespace MayaUtils {

bool getNodeFromName(const MString& nodeName, MObject& outNode)
{
    MStatus status;
    MSelectionList selList;

    status = selList.add(nodeName);
    if (status != MS::kSuccess) {
        return false;
    }

    status = selList.getDependNode(0, outNode);
    if (status != MS::kSuccess) {
        return false;
    }

    return true;
}

bool getDependencyNodeFromName(const MString& nodeName, MFnDependencyNode& outFnDep)
{
    MStatus status;
    MObject node;

    if (!getNodeFromName(nodeName, node)) {
        return false;
    }

    status = outFnDep.setObject(node);
    if (status != MS::kSuccess) {
        return false;
    }

    return true;
}

bool setAttributeValueFromString(MPlug& plug, const MObject& attr, const MString& value)
{
    MStatus status;
    MFn::Type apiType = attr.apiType();

    if (apiType == MFn::kNumericAttribute) {
        MFnNumericAttribute fnNum(attr);
        MFnNumericData::Type numType = fnNum.unitType();

        if (numType == MFnNumericData::kBoolean) {
            bool bValue = (value == "true" || value == "1");
            status = plug.setValue(bValue);
        } else if (numType == MFnNumericData::kInt || numType == MFnNumericData::kLong) {
            int iValue = value.asInt();
            status = plug.setValue(iValue);
        } else if (numType == MFnNumericData::kFloat) {
            float fValue = value.asFloat();
            status = plug.setValue(fValue);
        } else if (numType == MFnNumericData::kDouble) {
            double dValue = value.asDouble();
            status = plug.setValue(dValue);
        }
    } else if (apiType == MFn::kEnumAttribute) {
        MFnEnumAttribute fnEnum(attr);
        // Search for value from string name
        short enumValue = -1;
        bool found = false;

        // Scan all fields to find matching name
        short minValue, maxValue;
        status = fnEnum.getMin(minValue);
        if (status != MS::kSuccess) minValue = 0;
        status = fnEnum.getMax(maxValue);
        if (status != MS::kSuccess) maxValue = 255;

        for (short i = minValue; i <= maxValue; ++i) {
            MString fieldName = fnEnum.fieldName(i, &status);
            if (status == MS::kSuccess && fieldName == value) {
                enumValue = i;
                found = true;
                break;
            }
        }

        if (found) {
            status = plug.setValue(enumValue);
        } else {
            // If string name is not found, interpret as numeric value
            enumValue = (short)value.asInt();
            status = plug.setValue(enumValue);
        }
    } else if (apiType == MFn::kTypedAttribute) {
        status = plug.setValue(value);
    } else {
        status = plug.setValue(value);
    }

    return (status == MS::kSuccess);
}

} // namespace MayaUtils
