#include <maya/MFnPlugin.h>
#include <maya/MGlobal.h>
#include <maya/MStatus.h>
#include "ExtraAttrManagerCmd.h"
#include "ExtraAttrUI.h"

/**
 * @brief Plugin initialization function
 *
 * Called when Maya loads the plugin.
 * Registers commands and nodes.
 */
MStatus initializePlugin(MObject obj)
{
    MStatus status;
    MFnPlugin plugin(obj, "Extra Attribute Manager", "1.0.0", "Any");

    // Register MEL command
    status = plugin.registerCommand(
        ExtraAttrManagerCmd::commandName,
        ExtraAttrManagerCmd::creator,
        ExtraAttrManagerCmd::newSyntax
    );

    if (!status) {
        status.perror("registerCommand failed");
        return status;
    }

    // Use Python to add menu with deferred execution
    // Define the function first, then schedule it to run when Maya UI is ready
    MString pythonScript =
        "import maya.cmds as mc\n"
        "import maya.mel as mel\n"
        "\n"
        "def add_extra_attr_menu():\n"
        "    try:\n"
        "        # Remove old menu item if exists\n"
        "        if mc.menuItem('ExtraAttrEditorMenuItem', exists=True):\n"
        "            mc.deleteUI('ExtraAttrEditorMenuItem')\n"
        "        \n"
        "        # Get main Maya window\n"
        "        main_window = mel.eval('$tmp = $gMainWindow')\n"
        "        windows_menu = main_window + '|mainWindowMenu'\n"
        "        \n"
        "        # Check if Windows menu exists\n"
        "        if not mc.menu(windows_menu, exists=True):\n"
        "            # Retry later if menu not ready\n"
        "            mc.evalDeferred('add_extra_attr_menu()', lowestPriority=True)\n"
        "            return\n"
        "        \n"
        "        # Find General Editors menu item\n"
        "        menu_items = mc.menu(windows_menu, query=True, itemArray=True) or []\n"
        "        general_editors_item = None\n"
        "        for item in menu_items:\n"
        "            try:\n"
        "                label = mc.menuItem(item, query=True, label=True)\n"
        "                if label == 'General Editors':\n"
        "                    general_editors_item = item\n"
        "                    break\n"
        "            except:\n"
        "                continue\n"
        "        \n"
        "        if not general_editors_item:\n"
        "            # General Editors not found, retry later\n"
        "            mc.evalDeferred('add_extra_attr_menu()', lowestPriority=True)\n"
        "            return\n"
        "        \n"
        "        # Get the full path to General Editors submenu\n"
        "        general_editors_path = windows_menu + '|' + general_editors_item\n"
        "        \n"
        "        # Add to General Editors submenu (will appear at top, but that's OK)\n"
        "        mc.setParent(general_editors_path, menu=True)\n"
        "        mc.menuItem('ExtraAttrEditorMenuItem',\n"
        "                    label='Extra Attribute Editor',\n"
        "                    command='import maya.cmds as mc; mc.exAttrEditor(ui=True)',\n"
        "                    annotation='Edit custom attributes in the scene')\n"
        "        \n"
        "        print('Extra Attribute Editor menu added to Windows > General Editors successfully.')\n"
        "    except Exception as e:\n"
        "        print('Error adding Extra Attribute Editor menu: ' + str(e))\n"
        "\n"
        "def remove_extra_attr_menu():\n"
        "    if mc.menuItem('ExtraAttrEditorMenuItem', exists=True):\n"
        "        mc.deleteUI('ExtraAttrEditorMenuItem')\n"
        "\n"
        "# Schedule menu creation with deferred execution\n"
        "mc.evalDeferred('add_extra_attr_menu()', lowestPriority=True)\n";

    // Execute Python script to define functions and schedule execution
    status = MGlobal::executePythonCommand(pythonScript);

    MGlobal::displayInfo("Extra Attribute Editor plugin loaded successfully.");
    MGlobal::displayInfo("Menu: Windows > General Editors > Extra Attribute Editor");

    return status;
}

/**
 * @brief Plugin uninitialization function
 *
 * Called when Maya unloads the plugin.
 * Deregisters registered commands and nodes.
 */
MStatus uninitializePlugin(MObject obj)
{
    MStatus status;
    MFnPlugin plugin(obj);

    // Delete menu item using Python
    MString removePythonScript =
        "import maya.cmds as mc\n"
        "if mc.menuItem('ExtraAttrEditorMenuItem', exists=True):\n"
        "    mc.deleteUI('ExtraAttrEditorMenuItem')\n";
    MGlobal::executePythonCommand(removePythonScript);

    // Cleanup UI singleton
    ExtraAttrUI::destroyInstance();

    // Deregister MEL command
    status = plugin.deregisterCommand(ExtraAttrManagerCmd::commandName);

    if (!status) {
        status.perror("deregisterCommand failed");
        return status;
    }

    MGlobal::displayInfo("Extra Attribute Editor plugin unloaded.");

    return status;
}
