function Component() {
    // Target directory: user's Documents/TextSmith
    var docsDir = QStandardPaths.writableLocation(QStandardPaths.DocumentsLocation);
    var targetDir = docsDir + "/TextSmith";

    // Ensure the directory exists
    component.addOperation("Mkdir", targetDir);

    // Extract the zip file shipped with this component
    // "@TargetDir@" expands to the component’s installation directory
    component.addOperation("Extract",
                           "@TargetDir@/scripts.zip",
                           targetDir);
}
