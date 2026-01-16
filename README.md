# Extra Attribute Editor for Maya

A convenient plugin for managing custom attributes in Autodesk Maya

![Extra Attribute Editor](https://github.com/user-attachments/assets/45351b86-577b-45d6-abc0-229ceb06fcaf)

## How to Use

1. Download `exattr-maya.mll` from [Releases](https://github.com/cignoir/exattr-maya/releases)
2. Copy to your Maya plug-ins folder (e.g., `%USERPROFILE%\Documents\maya\2025\plug-ins\`)
3. Load the plugin via Maya's Plug-in Manager
4. Launch from menu: **Windows > General Editors > Extra Attribute Editor**
5. Or use MEL command: `exAttrEditor -ui;`

## Key Features

- High-speed scanning of custom attributes across the entire scene
- Batch editing of nodes with custom attributes
- Filter and sort functionality
- Select polygons assigned to materials with custom attributes
- **Extract Assigned Polygons**:
    - Right-click on a material node in the list.
    - Choose **Extract Assigned Polygons...** to separate geometry assigned to that material.
    - **Options**:
        - **New Object Name**: Name for the extracted mesh(es).
        - **Keep Original Polygons**: If unchecked, original faces are deleted.
        - **Assign Single Lambert Material**: Assigns a new default shader to extracted/remaining faces.
        - **Combine Meshes**: Merges multiple extracted meshes into a single object (preserves UVs, deletes history).
    - **Grouping**: Extracted objects are automatically placed in a new group (`[Name]_grp`) at the scene root.

## Specifications

- **Maya Version**: Maya 2025 or later
- **Platform**: Windows 10/11 (64-bit)
- **Performance**: 10,000 objects / 3 seconds
- **License**: MIT License

## Building from Source

For developers.

### Requirements

- Visual Studio 2022
- CMake 3.20 or later
- Maya 2025 SDK

### Build Steps

```cmd
build.bat
```

Custom Maya path:
```cmd
build.bat "D:\Autodesk\Maya2025"
```

Build output: `build/exattr-maya.mll`

## License

MIT License - See [LICENSE](LICENSE) file for details

© 2025 cignoir
