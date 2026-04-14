# Custom Fonts

This directory contains everything related to custom fonts: source `.ttf`
files, the generation script, generated `.c` bitmap fonts, and the font
license. Everything lives together so the license clearly covers all
derived files.

The generated `.c` files are committed to the repo so the build does not
depend on Node.js or `lv_font_conv`. You only need to regenerate if you
change font sizes, weights, or character sets.

## Current fonts

| File | Size | Weight | Characters | Used for |
|------|------|--------|------------|----------|
| `lv_font_montserrat_bold_116.c` | 116px | Bold | `+-= 0-9` | Life total, dice result |
| `lv_font_montserrat_regular_48.c` | 48px | Regular | `+-= 0-9` | Life preview total ("= xxx") |

## Regenerating fonts

Install `lv_font_conv` globally (one-time setup):

```bash
npm install -g lv_font_conv
```

Then run the generation script:

```bash
./generate_fonts.sh
```

This produces `.c` files alongside the source `.ttf` files. Rebuild the simulator to
test:

```bash
cd sim && make clean && make && ./generate_matrix.sh
```

## Adding a new font

1. Edit `generate_fonts.sh` and add a `generate_font` call:

   ```bash
   generate_font "lv_font_montserrat_bold_64" "$BOLD" 64 "$RANGE_DIGITS"
   ```

   Parameters: `output_name`, `font_file`, `size`, `character_range`.

2. To use a different character set, define a new range variable or pass
   one inline. Predefined ranges:
   - `$RANGE_DIGITS` — space, `+`, `-`, `0`-`9`, `=` (for numeric displays)
   - `$RANGE_ASCII` — printable ASCII 0x20-0x7F (for general text)

3. Run `./generate_fonts.sh` to produce the `.c` file.

4. Declare the font in `knobby/lv_conf.h` under `LV_FONT_CUSTOM_DECLARE`:

   ```c
   #define LV_FONT_CUSTOM_DECLARE \
       LV_FONT_DECLARE(lv_font_montserrat_bold_116) \
       LV_FONT_DECLARE(lv_font_montserrat_bold_48) \
       LV_FONT_DECLARE(lv_font_montserrat_bold_64)
   ```

5. Use it in code:

   ```c
   lv_obj_set_style_text_font(label, &lv_font_montserrat_bold_64, 0);
   ```

6. Commit the generated `.c` file.

## Font source files

- `Montserrat-Bold.ttf` — used for life total and dice result
- `Montserrat-Regular.ttf` — used for life preview total

Both are from the [Montserrat project](https://github.com/JulietaUla/Montserrat).

## License

The Montserrat `.ttf` source files and all generated `.c` bitmap font files
derived from them are licensed under the
[SIL Open Font License, Version 1.1](OFL.txt).
