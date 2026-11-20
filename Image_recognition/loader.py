import os
import random
import shutil
import yaml

# 1. Paths
source_dir = str(os.path.relpath(__file__).replace("loader.py",""))
output_dir = "yolo_dataset"
split_ratios = (0.8, 0.1, 0.1)  # train, valid, test

# 2. Read class names from classes.txt
with open(os.path.join(source_dir, "classes.txt"), "r") as f:
    classes = [line.strip() for line in f.readlines() if line.strip()]

# 3. Gather and shuffle image files
images_dir = os.path.join(source_dir, "images")
labels_dir = os.path.join(source_dir, "labels")
image_files = [f for f in os.listdir(images_dir) if f.endswith(('.jpg', '.jpeg', '.png'))]
random.shuffle(image_files)

# 4. Calculate split indices
total = len(image_files)
train_end = int(total * split_ratios[0])
valid_end = train_end + int(total * split_ratios[1])

splits = {
    "train": image_files[:train_end],
    "valid": image_files[train_end:valid_end],
    "test": image_files[valid_end:]
}

# 5. Create folders and copy files
for split_name, files in splits.items():
    os.makedirs(os.path.join(output_dir, split_name, "images"), exist_ok=True)
    os.makedirs(os.path.join(output_dir, split_name, "labels"), exist_ok=True)
    
    for img_file in files:
        # Copy image
        shutil.copy(
            os.path.join(images_dir, img_file),
            os.path.join(output_dir, split_name, "images", img_file)
        )
        # Copy matching label file
        label_file = os.path.splitext(img_file)[0] + ".txt"
        src_label = os.path.join(labels_dir, label_file)
        if os.path.exists(src_label):
            shutil.copy(
                src_label,
                os.path.join(output_dir, split_name, "labels", label_file)
            )

# 6. Create data.yaml
data_yaml = {
    "train": "../train/images",
    "val": "../valid/images",
    "test": "../test/images",
    "nc": len(classes),
    "names": classes
}

with open(os.path.join(output_dir, "data.yaml"), "w") as f:
    yaml.dump(data_yaml, f, default_flow_style=False)

print(f"Dataset ready at '{output_dir}' with {len(classes)} classes!")
