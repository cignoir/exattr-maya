# Extra Attribute Editor for Maya

A high-performance C++ plugin with Qt UI for managing Extra Attributes (custom/dynamic attributes) in Autodesk Maya.

<img width="1199" height="634" alt="image" src="https://github.com/user-attachments/assets/45351b86-577b-45d6-abc0-229ceb06fcaf" />

## Features

### Core Capabilities
- **Scene-wide scanning** - Quickly detect all custom attributes across large scenes (~10,000 objects)
- **Attribute overview** - Display attribute name, type, usage count, and array status
- **Node search** - Find all nodes using specific attributes
- **Live editing** - Edit attribute values individually or in batch
- **Attribute management** - Add or delete attributes with ease
- **Sort & Filter** - Organize and search through attributes efficiently

### UI Features
- **Left Panel**: Extra Attribute list
- **Right Panel**: Node list with values for selected attribute
- **Context Menus**: Right-click for quick operations
- **Real-time Editing**: Edit values directly in the table (double-click or F2)
- **Type-aware Widgets**: Enum dropdowns, number inputs, text fields, and checkboxes
- **Column Sorting**: Sort by any column in both tables
- **Advanced Filtering**: Filter by attribute name, node name, node type, or value

### MEL Command Interface

```mel
# Show UI
exAttrEditor -ui;

# Scan scene for extra attributes
exAttrEditor -scan;

# List all extra attributes
exAttrEditor -list;

# Get nodes with specific attribute
exAttrEditor -nodes "myCustomAttr";

# Edit attribute value
exAttrEditor -edit "pCube1" "myAttr" "100";

# Delete attribute
exAttrEditor -delete "pCube1" "oldAttr";

# Add new attribute
exAttrEditor -add "pCube1" "newAttr" "double";

# Close UI
exAttrEditor -closeUI;

# Show help
exAttrEditor -help;
```

## System Requirements

### Required
- **Maya**: Maya 2025 or later
- **OS**: Windows 10/11 (64-bit)
- **Compiler**: Visual Studio 2022 (MSVC 14.3 or later)
- **Build Tools**:
  - CMake 3.20 or later
  - MSBuild (included with Visual Studio 2022)

### Performance
- Handles scenes with up to ~10,000 objects
- Scan time: ~3 seconds for 10,000 objects
- UI updates: Real-time (<100ms)

## Building from Source

### Prerequisites

#### Visual Studio 2022
- Install Visual Studio 2022 (Community, Professional, or Enterprise)
- Select "Desktop development with C++" workload

#### CMake
```bash
# Download and install CMake
# https://cmake.org/download/
# Select "Add CMake to the system PATH" during installation
```

**Note**: MSBuild is included with Visual Studio 2022 and doesn't require separate installation.

### Build Steps

#### Standard Build (Maya 2025 in default location)
```cmd
build.bat
```

#### Custom Maya Path
```cmd
build.bat "D:\Autodesk\Maya2025"
```

### Build Output

Upon successful build, the plugin file will be generated at:
```
build/exattr-maya.mll
```

The build script automatically copies the plugin to `MAYA_PLUG_IN_PATH` if the environment variable is set.

## Installation

### Method 1: Using MAYA_PLUG_IN_PATH (Recommended)

Set the environment variable before building:
```cmd
set MAYA_PLUG_IN_PATH=C:\Users\YourName\Documents\maya\2025\plug-ins
```

The build script will automatically copy the plugin to this location.

### Method 2: User Plug-ins Directory

```cmd
# Copy plugin to Maya user plug-ins directory
copy build\exattr-maya.mll "%USERPROFILE%\Documents\maya\2025\plug-ins\"
```

### Method 3: Custom Directory

Use Maya's Plug-in Manager to load from any location.

## Usage

### Loading the Plugin

#### MEL Command
```mel
loadPlugin "exattr-maya.mll";
```

#### Plug-in Manager
1. Maya menu: **Windows > Settings/Preferences > Plug-in Manager**
2. Find `exattr-maya.mll` and check the box to load

### Opening the UI

#### Method 1: Menu (Recommended)
After loading the plugin, it automatically adds a menu item:

1. Navigate to: **Windows > Extra Attribute Editor**

#### Method 2: MEL Command
```mel
exAttrEditor -ui;
```

### Basic Workflow

1. **Scan Scene**: Click "Scan Scene" button
2. **Select Attribute**: Choose an Extra Attribute from the left panel
3. **View Nodes**: Right panel shows all nodes with that attribute
4. **Edit Values**: Double-click value to edit directly
5. **Batch Operations**:
   - Select multiple nodes with right-click
   - Choose "Batch Edit Selected Nodes..." for bulk editing

### Advanced Features

#### Adding Attributes
1. Select target node(s) in Maya
2. Right-click in left panel > "Add Attribute to Selected Nodes..."
3. Enter attribute name and type

#### Deleting Attributes
1. Select node with attribute in right panel
2. Right-click > "Delete Attribute from This Node..."

#### Selecting Nodes
1. Select node in right panel
2. Right-click > "Select Node in Maya"
3. Node becomes selected in Maya viewport

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.
