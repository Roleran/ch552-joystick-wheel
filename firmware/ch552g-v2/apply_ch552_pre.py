Import("env")

# 这些参数必须在平台脚本创建编译目标之前写进环境。
env.Append(
    CPPDEFINES=[
        "CH552",
        ("USER_USB_RAM", 148),
        ("F_EXT_OSC", 0),
    ],
    CCFLAGS=["--model-large"],
)
