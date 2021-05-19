function Component(){}
Component.prototype.createOperations = function()
{
    component.createOperations();
    if (systemInfo.productType === "windows") {
        component.addOperation("CreateShortcut", "@TargetDir@/AstrocatApp.exe", "@StartMenuDir@/AstrocatApp.lnk",
            "workingDirectory=@TargetDir@");
    }
}
