import os

target_dir = 'lipo-result'
os.makedirs(target_dir, exist_ok=True)
arm64_dir = 'arm64'
x86_64_dir = 'x86_64'

for root, dirs, files in os.walk(arm64_dir):
    for file in files:
        if file.endswith('.dylib'):
            arm64_path = os.path.join(root, file)
            x86_64_path = os.path.join(x86_64_dir, root[len(arm64_dir)+1:], file)
            target_path = os.path.join(target_dir, root[len(arm64_dir)+1:], file)
            os.makedirs(os.path.dirname(target_path), exist_ok=True)
            os.system(f'lipo -create {arm64_path} {x86_64_path} -output {target_path}')
