import os
import sys

def rwildcard(directory, pattern):
    sources = []
    for root, dirs, files in os.walk(directory):
        for file in files:
            if file.endswith(pattern):
                sources.append(os.path.join(root, file).replace('\\', '/'))
    return sources

def main():
    if len(sys.argv) < 3:
        print("Usage: gen_web_sources.py <lvgl_path> <knobby_path>")
        sys.exit(1)
        
    lvgl_path = sys.argv[1]
    knobby_path = sys.argv[2]
    
    sources = [
        'sim_sdl2.c',
        'sim_stubs.c'
    ]
    
    # Knobby sources — auto-discovered (mirrors native Makefile rwildcard logic)
    # Top-level .c files (e.g. knob.c)
    for f in os.listdir(knobby_path):
        if f.endswith('.c'):
            sources.append(os.path.join(knobby_path, f).replace('\\', '/'))

    # All .c files under src/ (recursively — includes fonts/)
    sources.extend(rwildcard(os.path.join(knobby_path, 'src'), '.c'))
        
    # LVGL
    sources.extend(rwildcard(os.path.join(lvgl_path, 'src'), '.c'))
    
    with open('web_sources.txt', 'w') as f:
        f.write(' '.join(sources))

if __name__ == '__main__':
    main()
