#!/bin/bash
# Run OIDC Qt Application with proper environment setup

# Set up environment for Qt WebEngine
export LD_LIBRARY_PATH="/home/coonal/Qt/5.15.2/gcc_64/lib:${LD_LIBRARY_PATH}"
export QT_PLUGIN_PATH="/home/coonal/Qt/5.15.2/gcc_64/plugins"
export QML_IMPORT_PATH="/home/coonal/Qt/5.15.2/gcc_64/qml"

# Run the application
./build/oidc-client-qt "$@"
