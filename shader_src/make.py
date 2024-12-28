import glob
import os
import shutil
import subprocess
import sys

# get shaderc from env "SHADERC"
shaderc = os.getenv("SHADERC")


def should_recompile_shader(src, dst):
    if not os.path.exists(dst):
        return True
    return os.path.getmtime(src) > os.path.getmtime(dst)


def compile_shader(src, dst, type, platform, profile):
    os.makedirs(os.path.dirname(dst), exist_ok=True)
    subprocess.run(
        [shaderc, "-f", src, "-o", dst, "--platform", platform, "--type", type, "--profile", profile, "-O 3", "-i", "."]
    )


def compile_all_shaders():
    # fs_*.sc : fragment shaders
    # vs_*.sc : vertex shaders

    # glob recursively
    fs_shaders = glob.glob("**/fs_*.sc", recursive=True)
    vs_shaders = glob.glob("**/vs_*.sc", recursive=True)

    for fs_shader in fs_shaders:
        if fs_shader.endswith("def.sc"):
            continue
        print(fs_shader)
        dst = "../shaders/metal/" + fs_shader.replace(".sc", ".bin")
        if should_recompile_shader(fs_shader, dst):
            compile_shader(fs_shader, dst, "f", "osx", "metal")
        dst = "../shaders/spirv/" + fs_shader.replace(".sc", ".bin")
        if should_recompile_shader(fs_shader, dst):
            compile_shader(fs_shader, dst, "f", "windows", "spirv")

    for vs_shader in vs_shaders:
        if vs_shader.endswith("def.sc"):
            continue
        print(vs_shader)
        dst = "../shaders/metal/" + vs_shader.replace(".sc", ".bin")
        if should_recompile_shader(vs_shader, dst):
            compile_shader(vs_shader, dst, "v", "osx", "metal")
        dst = "../shaders/spirv/" + vs_shader.replace(".sc", ".bin")
        if should_recompile_shader(vs_shader, dst):
            compile_shader(vs_shader, dst, "v", "windows", "spirv")


if __name__ == "__main__":
    args = sys.argv[1:]
    if len(args) > 0 and args[0] == "clean":
        shutil.rmtree("../shaders/metal")
        shutil.rmtree("../shaders/spirv")
    else:
        compile_all_shaders()
