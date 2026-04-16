#!/bin/bash
# Generate the full screenshot matrix.
# Run from the sim/ directory: ./generate_matrix.sh
set -e

SIM=./knobby_sim
OUT=screenshots
COUNT=0
FILES=()

mkdir -p "$OUT"

shot() {
    local filename="$1"
    shift
    $SIM --outdir "$OUT" --output "$filename" "$@"
    COUNT=$((COUNT + 1))
    FILES+=("$filename")
}

# ============================================================
# 1. Life preview deltas — 1p mode
# ============================================================
for delta in +444 -444 +1; do
    tag=$(echo "$delta" | tr '+' 'p' | tr '-' 'n')
    shot "1p_preview_${tag}.png" --screen 1p --track 1 \
        --preview-delta "$delta" --preview-player -1
done

# ============================================================
# 2. Life preview deltas — multiplayer modes × orientations
# ============================================================
# Worst-case (+444) in every quadrant × orientation to catch overlap/clipping
for track in 2 3 4; do
    max_player=$((track - 1))
    for orient in 0 1 2; do
        orient_name=("absolute" "centric" "tabletop")
        oname=${orient_name[$orient]}
        for player in $(seq 0 $max_player); do
            shot "${track}p_${oname}_p${player}_preview_p444.png" \
                --screen ${track}p --track "$track" --orientation "$orient" \
                --preview-delta +444 --preview-player "$player"
        done
    done
done
# Remaining deltas for player 0 at absolute orientation (formatting coverage)
for track in 2 3 4; do
    for delta in -444 +1 -1 +12 -12; do
        tag=$(echo "$delta" | tr '+' 'p' | tr '-' 'n')
        shot "${track}p_absolute_p0_preview_${tag}.png" \
            --screen ${track}p --track "$track" --orientation 0 \
            --preview-delta "$delta" --preview-player 0
    done
done

# ============================================================
# 3. Life totals at specific values — all player modes × orientations
# ============================================================
# All life values at one orientation (color tier coverage)
for life in -5 0 20 40 444; do
    ltag=$life
    [ "$life" -lt 0 ] && ltag="n${life#-}"
    shot "1p_life${ltag}.png" --screen 1p --track 1 \
        --starting-life 40 --life "$life"

    # 1p — custom color override
    shot "1p_custom_life${ltag}.png" --screen 1p --track 1 \
        --starting-life 40 --life "$life" \
        --player-override 1,0,0,0 --player-colors 4,0,0,0

    # multiplayer — player colors
    for track in 2 3 4; do
        life_csv=$(printf "%s" "$life"; for j in $(seq 2 $track); do printf ",%s" "$life"; done)
        shot "${track}p_absolute_life${ltag}.png" \
            --screen ${track}p --track "$track" --orientation 0 \
            --starting-life 40 --life "$life_csv"
    done
done
# Worst-case widths (444, -5) across all orientations for clipping detection
for life in -5 444; do
    ltag=$life
    [ "$life" -lt 0 ] && ltag="n${life#-}"
    for track in 2 3 4; do
        life_csv=$(printf "%s" "$life"; for j in $(seq 2 $track); do printf ",%s" "$life"; done)
        for orient in 1 2; do
            orient_name=("absolute" "centric" "tabletop")
            oname=${orient_name[$orient]}
            shot "${track}p_${oname}_life${ltag}.png" \
                --screen ${track}p --track "$track" --orientation "$orient" \
                --starting-life 40 --life "$life_csv"
        done
    done
done

# ============================================================
# 4. Life-color mode — multiplayer × orientations × mixed life
# ============================================================
for track in 2 3 4; do
    case "$track" in
        2) life_csv="5,35" ;;
        3) life_csv="5,20,35" ;;
        4) life_csv="5,15,35,50" ;;
    esac
    for orient in 0 1 2; do
        orient_name=("absolute" "centric" "tabletop")
        oname=${orient_name[$orient]}
        shot "${track}p_${oname}_lifecolor_mixed.png" \
            --screen ${track}p --track "$track" --orientation "$orient" \
            --color-mode 1 --starting-life 40 --life "$life_csv"
    done
done

# ============================================================
# 5. Selected player (no preview) — multiplayer, one orientation
# ============================================================
for track in 2 3 4; do
    max_player=$((track - 1))
    for player in $(seq 0 $max_player); do
        shot "${track}p_absolute_selected_p${player}.png" \
            --screen ${track}p --track "$track" --orientation 0 \
            --selected "$player"
    done
done

# ============================================================
# 6. Random counters — multiplayer × orientations + 1p
# ============================================================
shot "1p_counters.png" --screen 1p --random-counters --auto-eliminate 0
for track in 2 3 4; do
    for orient in 0 1 2; do
        orient_name=("absolute" "centric" "tabletop")
        oname=${orient_name[$orient]}
        shot "${track}p_${oname}_counters.png" \
            --screen ${track}p --track "$track" --orientation "$orient" \
            --random-counters --auto-eliminate 0
    done
done

# ============================================================
# 7. Brightness at different levels
# ============================================================
for bri in 1 50 100; do
    shot "brightness_${bri}pct.png" --screen brightness --brightness "$bri"
done

# ============================================================
# 8. Counter-edit for each counter type
# ============================================================
for ct in 0 1 2 3; do
    ct_name=("cmd_tax" "partner_tax" "poison" "experience")
    shot "counter_edit_${ct_name[$ct]}.png" --screen counter-edit \
        --counter-type "$ct" --counter-value $((RANDOM % 50 + 1)) \
        --counter-player $((RANDOM % 4))
done

# ============================================================
# 9. Damage screen with non-zero damage
# ============================================================
shot "damage_nonzero.png" --screen damage --names Maya,Leah,Kyle \
    --enemy-damage 7,3,12

# ============================================================
# 10. Select screen with accumulated commander damage
# ============================================================
# 1-7 enemies (2-8 players)
shot "select_2p.png" --screen select --players 2 --track 1 \
    --names Maya,Leah --enemy-damage 9
shot "select_3p.png" --screen select --players 3 --track 1 \
    --names Maya,Leah,Kyle --enemy-damage 14,6
shot "select_4p.png" --screen select --players 4 --track 4 \
    --names Maya,Leah,Kyle,Devin --enemy-damage 14,6,21
shot "select_5p.png" --screen select --players 5 --track 4 \
    --names Maya,Leah,Kyle,Devin,Riku --enemy-damage 3,0,8,0
shot "select_6p.png" --screen select --players 6 --track 4 \
    --names Maya,Leah,Kyle,Devin,Riku,Sarah --enemy-damage 5,0,12,0,3
shot "select_7p.png" --screen select --players 7 --track 4 \
    --names Maya,Leah,Kyle,Devin,Riku,Sarah,Nolan \
    --enemy-damage 5,0,12,0,3,0
shot "select_8p.png" --screen select --players 8 --track 4 \
    --names Maya,Leah,Kyle,Devin,Riku,Sarah,Nolan,Tomoko \
    --enemy-damage 5,0,12,0,3,0,7
# select screen with an eliminated player (Kyle at 0 life)
shot "select_eliminated.png" --screen select --players 5 --track 4 \
    --names Maya,Leah,Kyle,Devin,Riku --enemy-damage 3,0,21,0 \
    --life 40,40,0,40 --auto-eliminate 1

# ============================================================
# 11. Custom-life with non-default value
# ============================================================
shot "custom_life_123.png" --screen custom-life --starting-life 123

# ============================================================
# 12. Game-mode with non-default settings
# ============================================================
shot "game_mode_2p_20.png" --screen game-mode --players 2 --track 2 --starting-life 20
shot "game_mode_3p_30.png" --screen game-mode --players 3 --track 3 --starting-life 30
shot "game_mode_8p_t4.png" --screen game-mode --players 8 --track 4 --starting-life 40

# ============================================================
# 13. Per-player color mode
# ============================================================
for track in 2 3 4; do
    for orient in 0 1 2; do
        orient_name=("absolute" "centric" "tabletop")
        oname=${orient_name[$orient]}
        case "$track" in
            2) colors="5,9";       overrides="1,1" ;;
            3) colors="5,9,0";     overrides="1,1,0" ;;
            4) colors="0,5,7,9";   overrides="0,1,1,1" ;;
        esac
        shot "${track}p_${oname}_perplayer.png" \
            --screen ${track}p --track "$track" --orientation "$orient" \
            --player-colors "$colors" --player-override "$overrides"
    done
done

# ============================================================
# 14. Color menu and picker screens
# ============================================================
shot "color_menu.png" --screen color-menu --menu-player 0
shot "color_picker.png" --screen color-picker --menu-player 0

# ============================================================
# 15. Settings pages with different toggle states
# ============================================================
for dim in 0 1 2 3; do
    dim_name=("off" "15s" "30s" "60s")
    shot "settings_menu_dim_${dim_name[$dim]}.png" --screen settings-menu --auto-dim "$dim"
done

for cm in 0 1; do
    cm_name=("player" "life")
    shot "settings_more_cm${cm_name[$cm]}.png" --screen settings-more --color-mode "$cm"
done

for dt in 0 1 2 3; do
    dt_name=("never" "5s" "15s" "30s")
    shot "settings_more_ds${dt_name[$dt]}.png" --screen settings-more --deselect "$dt"
done

for rot in 0 1 2; do
    rot_name=("absolute" "centric" "tabletop")
    shot "settings_more_orient_${rot_name[$rot]}.png" --screen settings-more --orientation "$rot"
done

for ae in 0 1; do
    ae_name=("aeoff" "aeon")
    shot "settings_more_${ae_name[$ae]}.png" --screen settings-more --auto-eliminate "$ae"
done

# ============================================================
# 14. All-damage screen with random value
# ============================================================
shot "all_damage_random.png" --screen all-damage \
    --all-damage-value $((RANDOM % 30 + 1))

# ============================================================
# 15. Player-menu for random player
# ============================================================
shot "player_menu_p$((RANDOM % 4)).png" --screen player-menu \
    --menu-player $((RANDOM % 4))

# ============================================================
# 16. Battery at random legal voltage
# ============================================================
# Pick from realistic range: 3.35V (0%) to 4.18V (100%)
voltages=("3.40" "3.55" "3.74" "3.96" "4.18")
v=${voltages[$((RANDOM % ${#voltages[@]}))]}
shot "battery_${v}v.png" --screen battery --battery-voltage "$v"

# ============================================================
# 17. Dice with a result
# ============================================================
shot "dice_$((RANDOM % 20 + 1)).png" --screen dice --dice $((RANDOM % 20 + 1))

# ============================================================
# 18. Event log with random data
# ============================================================
shot "damage_log_random.png" --screen damage-log --random-log

# ============================================================
# 19. Timer overlay — 1p mode at worst-case life totals
# ============================================================
for life in 0 444; do
    shot "1p_timer_life${life}.png" --screen 1p --track 1 \
        --starting-life 40 --life "$life" \
        --turn-number $((RANDOM % 31)) --turn-elapsed $((RANDOM % 21600 * 1000))
done

# Timer with preview delta +444
shot "1p_timer_preview_p444.png" --screen 1p --track 1 \
    --preview-delta +444 --preview-player -1 \
    --turn-number $((RANDOM % 31)) --turn-elapsed $((RANDOM % 21600 * 1000)) \
    --random-counters

# ============================================================
# 20. Intro screen
# ============================================================
shot "intro.png" --screen intro

# ============================================================
# 21. Mana Pool
# ============================================================
shot "mana_empty.png" --screen mana
shot "mana_sel_white.png" --screen mana --mana 44,7,0,12,3,21 --mana-selected 0
shot "mana_sel_blue.png" --screen mana --mana 2,44,5,0,18,9 --mana-selected 1
shot "mana_sel_black.png" --screen mana --mana 8,3,44,15,0,6 --mana-selected 2
shot "mana_sel_red.png" --screen mana --mana 0,11,4,44,7,33 --mana-selected 3
shot "mana_sel_green.png" --screen mana --mana 14,0,9,3,44,1 --mana-selected 4
shot "mana_sel_colorless.png" --screen mana --mana 6,22,1,8,0,44 --mana-selected 5
shot "mana_3digit.png" --screen mana --mana 444,77,0,123,8,444 --mana-selected 0
shot "mana_preview_pos.png" --screen mana --mana 5,3,0,7,2,1 --mana-selected 1 --mana-delta 12
shot "mana_preview_neg.png" --screen mana --mana 5,3,0,7,2,1 --mana-selected 3 --mana-delta -4
shot "mana_preview_max.png" --screen mana --mana 0,0,0,0,0,0 --mana-selected 0 --mana-delta 444
shot "mana_preview_clamp.png" --screen mana --mana 3,0,0,0,0,0 --mana-selected 0 --mana-delta -99

# ============================================================
# Generate index.html
# ============================================================
INDEX="$OUT/index.html"
cat > "$INDEX" << 'HEADER'
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Knobby Screenshot Matrix</title>
<style>
  body { font-family: sans-serif; max-width: 1200px; margin: 2rem auto; padding: 0 1rem; background: #f5f5f5; }
  h1 { margin-bottom: 0.25rem; }
  .count { color: #666; margin-bottom: 1.5rem; }
  h2 { margin-top: 2rem; border-bottom: 1px solid #ccc; padding-bottom: 0.25rem; }
  .grid { display: grid; grid-template-columns: repeat(auto-fill, minmax(180px, 1fr)); gap: 1rem; }
  .item { text-align: center; }
  .item img { max-width: 100%; border: 1px solid #ccc; border-radius: 4px; background: #fff; }
  .item a { display: block; margin-top: 0.25rem; word-break: break-all; font-size: 0.75rem; color: #333; }
</style>
</head>
<body>
<h1>Knobby Screenshot Matrix</h1>
HEADER

echo "<p class=\"count\">$COUNT screenshots</p>" >> "$INDEX"

write_section() {
    local title="$1"
    shift
    local files=("$@")
    if [ ${#files[@]} -eq 0 ]; then return; fi
    echo "<h2>$title</h2>" >> "$INDEX"
    echo '<div class="grid">' >> "$INDEX"
    for f in "${files[@]}"; do
        echo "  <div class=\"item\"><a href=\"$f\"><img src=\"$f\" alt=\"$f\"></a><a href=\"$f\">$f</a></div>" >> "$INDEX"
    done
    echo '</div>' >> "$INDEX"
}

# Sort files into sections
SEC_1P_PREV=(); SEC_2P_PREV=(); SEC_3P_PREV=(); SEC_4P_PREV=()
SEC_LIFE=(); SEC_LIFECOLOR=(); SEC_PERPLAYER=(); SEC_SELECTED=(); SEC_COUNTERS=()
SEC_BRIGHT=(); SEC_COUNTER_EDIT=(); SEC_DAMAGE=(); SEC_SETTINGS=()
SEC_TIMER=()
SEC_MANA=()
SEC_OTHER=()

for f in "${FILES[@]}"; do
    case "$f" in
        1p_preview_*)         SEC_1P_PREV+=("$f") ;;
        2p_*_preview_*)       SEC_2P_PREV+=("$f") ;;
        3p_*_preview_*)       SEC_3P_PREV+=("$f") ;;
        4p_*_preview_*)       SEC_4P_PREV+=("$f") ;;
        1p_timer_*)           SEC_TIMER+=("$f") ;;
        *_lifecolor_*)        SEC_LIFECOLOR+=("$f") ;;
        *_perplayer*)         SEC_PERPLAYER+=("$f") ;;
        *_life[0-9n]*)        SEC_LIFE+=("$f") ;;
        *_selected_*)         SEC_SELECTED+=("$f") ;;
        *_counters.png)       SEC_COUNTERS+=("$f") ;;
        brightness_*)         SEC_BRIGHT+=("$f") ;;
        counter_edit_*)       SEC_COUNTER_EDIT+=("$f") ;;
        damage_*|select_*|all_damage_*) SEC_DAMAGE+=("$f") ;;
        settings_*)           SEC_SETTINGS+=("$f") ;;
        mana_*)               SEC_MANA+=("$f") ;;
        *)                    SEC_OTHER+=("$f") ;;
    esac
done

write_section "1-Player Life Preview" "${SEC_1P_PREV[@]}"
write_section "2-Player Life Preview" "${SEC_2P_PREV[@]}"
write_section "3-Player Life Preview" "${SEC_3P_PREV[@]}"
write_section "4-Player Life Preview" "${SEC_4P_PREV[@]}"
write_section "1-Player Timer Overlay" "${SEC_TIMER[@]}"
write_section "Life Totals (Player Colors)" "${SEC_LIFE[@]}"
write_section "Life Totals (Life Colors)" "${SEC_LIFECOLOR[@]}"
write_section "Life Totals (Per-Player Colors)" "${SEC_PERPLAYER[@]}"
write_section "Selected Player" "${SEC_SELECTED[@]}"
write_section "Player Counters" "${SEC_COUNTERS[@]}"
write_section "Brightness" "${SEC_BRIGHT[@]}"
write_section "Counter Edit" "${SEC_COUNTER_EDIT[@]}"
write_section "Damage / Select" "${SEC_DAMAGE[@]}"
write_section "Settings" "${SEC_SETTINGS[@]}"
write_section "Mana Pool" "${SEC_MANA[@]}"
write_section "Other" "${SEC_OTHER[@]}"

echo '</body></html>' >> "$INDEX"

echo ""
echo "Generated $COUNT screenshots in $OUT/"
echo "Index: $OUT/index.html"
