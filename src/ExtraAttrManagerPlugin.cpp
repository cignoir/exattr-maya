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
    MFnPlugin plugin(obj, "Extra Attribute Manager", "1.0.4", "Any");

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

    // Maya's Windows menu is lazy-loaded.  Force it to build via
    // buildDeferredMenus, then locate "General Editors" and add our item.
    // We retry a few times in case Maya UI isn't fully ready yet.
    MString pythonScript =
        "import maya.cmds as mc\n"
        "import maya.mel as mel\n"
        "\n"
        "def _exattr_add_menu(attempt=0):\n"
        "    try:\n"
        "        if mc.menuItem('ExtraAttrEditorMenuItem', exists=True):\n"
        "            print('[ExtraAttr] menu item already exists')\n"
        "            return\n"
        "        main_window = mel.eval('$tmp = $gMainWindow')\n"
        "        windows_menu = main_window + '|mainWindowMenu'\n"
        "        if not mc.menu(windows_menu, exists=True):\n"
        "            if attempt < 20:\n"
        "                mc.evalDeferred(lambda: _exattr_add_menu(attempt + 1), lowestPriority=True)\n"
        "            else:\n"
        "                print('[ExtraAttr] Windows menu not found after retries')\n"
        "            return\n"
        "        # Force Maya to build the lazy-loaded menu contents\n"
        "        try:\n"
        "            mel.eval('buildDeferredMenus')\n"
        "        except Exception:\n"
        "            pass\n"
        "        menu_items = mc.menu(windows_menu, query=True, itemArray=True) or []\n"
        "        if not menu_items:\n"
        "            if attempt < 20:\n"
        "                mc.evalDeferred(lambda: _exattr_add_menu(attempt + 1), lowestPriority=True)\n"
        "            else:\n"
        "                print('[ExtraAttr] Windows menu items still empty after retries')\n"
        "            return\n"
        "        # Match by localized labels (English / Japanese / Chinese)\n"
        "        target_labels = (\n"
        "            'General Editors',\n"
        "            '\\u4e00\\u822c\\u30a8\\u30c7\\u30a3\\u30bf',\n"
        "            '\\u901a\\u7528\\u7f16\\u8f91\\u5668',\n"
        "        )\n"
        "        general_editors_item = None\n"
        "        for item in menu_items:\n"
        "            try:\n"
        "                if not mc.menuItem(item, query=True, subMenu=True):\n"
        "                    continue\n"
        "                label = mc.menuItem(item, query=True, label=True) or ''\n"
        "                if label in target_labels:\n"
        "                    general_editors_item = item\n"
        "                    break\n"
        "            except Exception:\n"
        "                continue\n"
        "        if not general_editors_item:\n"
        "            print('[ExtraAttr] General Editors submenu not found')\n"
        "            return\n"
        "        general_editors_path = windows_menu + '|' + general_editors_item\n"
        "        mc.setParent(general_editors_path, menu=True)\n"
        "        mc.menuItem('ExtraAttrEditorMenuItem',\n"
        "                    label='Extra Attribute Editor',\n"
        "                    command='import maya.cmds as mc; mc.exAttrEditor(ui=True)',\n"
        "                    annotation='Edit custom attributes in the scene')\n"
        "        print('[ExtraAttr] menu added successfully')\n"
        "    except Exception as e:\n"
        "        import traceback\n"
        "        print('[ExtraAttr] Error: ' + str(e))\n"
        "        traceback.print_exc()\n"
        "\n"
        "def remove_extra_attr_menu():\n"
        "    if mc.menuItem('ExtraAttrEditorMenuItem', exists=True):\n"
        "        mc.deleteUI('ExtraAttrEditorMenuItem')\n"
        "\n"
        "mc.evalDeferred(lambda: _exattr_add_menu(0), lowestPriority=True)\n";

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
