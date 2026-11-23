import os
import math

def generate_nv12_video(filename, width=1280, height=720, num_frames=30):
    print(f"Generating synthetic NV12 test video: {filename} ({width}x{height}, {num_frames} frames)...")
    y_size = width * height
    uv_size = width * (height // 2)
    
    with open(filename, "wb") as f:
        for frame_idx in range(num_frames):
            y_plane = bytearray(y_size)
            uv_plane = bytearray(uv_size)
            
            # Moving circle center
            cx = int(width / 2 + math.sin(frame_idx * 0.2) * (width * 0.25))
            cy = int(height / 2 + math.cos(frame_idx * 0.2) * (height * 0.25))
            radius = 80
            
            # Generate Y plane with sharp geometric patterns and gradients
            for y in range(height):
                row_offset = y * width
                for x in range(width):
                    # Background diagonal stripes (high frequency detail)
                    stripe = 200 if ((x // 16) + (y // 16)) % 2 == 0 else 50
                    
                    # Moving circular target
                    dx = x - cx
                    dy = y - cy
                    dist_sq = dx * dx + dy * dy
                    if dist_sq < radius * radius:
                        # Radial gradient inside circle
                        lum = int(255 * (1.0 - math.sqrt(dist_sq) / radius))
                        y_plane[row_offset + x] = min(255, max(16, lum))
                    else:
                        y_plane[row_offset + x] = stripe
            
            # Generate UV plane (128 is neutral chroma)
            for y in range(height // 2):
                row_offset = y * width
                for x in range(0, width, 2):
                    # Slightly tint circle area with color
                    dx = x - cx
                    dy = (y * 2) - cy
                    if (dx * dx + dy * dy) < radius * radius:
                        uv_plane[row_offset + x] = 80   # U
                        uv_plane[row_offset + x + 1] = 200 # V
                    else:
                        uv_plane[row_offset + x] = 128     # U neutral
                        uv_plane[row_offset + x + 1] = 128 # V neutral
            
            f.write(y_plane)
            f.write(uv_plane)
            
    print(f"Generated {filename} successfully ({os.path.getsize(filename)} bytes).")

if __name__ == "__main__":
    out_dir = r"C:\Users\kshvs\Sources\Antigravity\RTX_Video_SDK_v1.1.0\RTX_DLSS5_Studio\data"
    os.makedirs(out_dir, exist_ok=True)
    out_path = os.path.join(out_dir, "NV12_1280_720_1280_30_synthetic_target.yuv")
    generate_nv12_video(out_path, 1280, 720, 30)
