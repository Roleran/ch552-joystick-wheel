import os

Import("env")

# USB DMA 缓冲固定在 xram 0..147，变量必须从 148 开始，否则会盖住端点缓冲。
env.Append(LINKFLAGS=["--model-large", "--xram-loc", "148"])

# 平台默认用 sdld，不认识 --xram-loc。改成 sdcc 做链接，才会把变量放到 xram 148 之后。
env.Replace(LD="sdcc", LINKCOM="$LD -o $TARGET $LINKFLAGS $SOURCES")


def _parse_hex(path):
    upper = 0
    highest = 0
    image = {}
    with open(path, "r", encoding="ascii") as handle:
        for raw in handle:
            line = raw.strip()
            if not line:
                continue
            if not line.startswith(":") or len(line) < 11:
                raise ValueError("not an Intel HEX line: %s" % line)
            count = int(line[1:3], 16)
            offset = int(line[3:7], 16)
            kind = int(line[7:9], 16)
            payload = line[9 : 9 + count * 2]
            if len(payload) != count * 2:
                raise ValueError("truncated Intel HEX record")
            if kind == 0x04 and count == 2:
                upper = int(payload, 16) << 16
            elif kind == 0x00 and count:
                start = upper + offset
                data = bytes.fromhex(payload)
                for index, value in enumerate(data):
                    image[start + index] = value
                end = start + count
                if end > highest:
                    highest = end
            elif kind == 0x01:
                break
    if not image:
        raise ValueError("hex has no data records")
    return highest, image


def _require_ljmp(image, address, name):
    if image.get(address) != 0x02:
        raise ValueError("%s vector at 0x%04X is not LJMP" % (name, address))


def check_hex(source, target, env):
    path = str(target[0])
    highest, image = _parse_hex(path)
    # CH552 引导程序从 0x3800 开始，用户代码不能写进去。
    if highest > 0x3800:
        raise ValueError("firmware overlaps bootloader: end=0x%X" % highest)
    _require_ljmp(image, 0x0000, "reset")
    _require_ljmp(image, 0x000B, "timer0")
    _require_ljmp(image, 0x0043, "usb")
    copy_to = os.path.join(env["PROJECT_DIR"], "ch552e-v1.hex")
    with open(path, "rb") as src, open(copy_to, "wb") as dst:
        dst.write(src.read())
    print("CH552 hex ok, end=0x%X, copied to ch552e-v1.hex" % highest)


env.AddPostAction("$BUILD_DIR/${PROGNAME}.hex", check_hex)
