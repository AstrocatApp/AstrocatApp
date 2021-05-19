function Component(){}
Component.prototype.createOperations = function()
{
    component.createOperations();
    if (systemInfo.productType === "windows")
    {
        // return value 3010 means it need a reboot, but in most cases it is not needed for running Qt application
        // return value 5100 means there's a newer version of the runtime already installed
        component.addOperation("Execute", "{0,3010,1638,5100}", "@TargetDir@\\vc_redist.x64.exe", "/quiet", "/norestart");
        component.addOperation("Delete", "@TargetDir@\\vc_redist.x64.exe");
    }
}
