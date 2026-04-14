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
for delta in +999 -999 +12 -12 +1 -1; do
    tag=$(echo "$delta" | tr '+' 'p' | tr '-' 'n')
    shot "1p_preview_${tag}.png" --screen 1p --track 1 \
        --preview-delta "$delta" --preview-player -1
done

# ============================================================
# 2. Life preview deltas — multiplayer modes × orientations
# ============================================================
for track in 2 3 4; do
    max_player=$((track - 1))
    for orient in 0 1 2; do
        orient_name=("absolute" "centric" "tabletop")
        oname=${orient_name[$orient]}
        for player in $(seq 0 $max_player); do
            for delta in +999 -999 +12 -12 +1 -1; do
                tag=$(echo "$delta" | tr '+' 'p' | tr '-' 'n')
                shot "${track}p_${oname}_p${player}_preview_${tag}.png" \
                    --screen ${track}p --track "$track" --orientation "$orient" \
                    --preview-delta "$delta" --preview-player "$player"
            done
        done
    done
done

# ============================================================
# 3. Life totals at specific values — all player modes × orientations
# ============================================================
for life in 0 20 40 999; do
    # 1p
    shot "1p_life${life}.png" --screen 1p --track 1 \
        --starting-life "$life" --life "$life"

    # multiplayer — player colors
    for track in 2 3 4; do
        life_csv=$(printf "%s" "$life"; for j in $(seq 2 $track); do printf ",%s" "$life"; done)
        for orient in 0 1 2; do
            orient_name=("absolute" "centric" "tabletop")
            oname=${orient_name[$orient]}
            shot "${track}p_${oname}_life${life}.png" \
                --screen ${track}p --track "$track" --orientation "$orient" \
                --starting-life "$life" --life "$life_csv"
        done
    done
done

# ============================================================
# 4. Life-color mode — multiplayer × orientations × varied life
# ============================================================
for track in 2 3 4; do
    for orient in 0 1 2; do
        orient_name=("absolute" "centric" "tabletop")
        oname=${orient_name[$orient]}

        # All at 40 (green)
        life_csv=$(printf "40"; for j in $(seq 2 $track); do printf ",40"; done)
        shot "${track}p_${oname}_lifecolor_40.png" \
            --screen ${track}p --track "$track" --orientation "$orient" \
            --color-mode 1 --starting-life 40 --life "$life_csv"

        # Mixed: each player at a different tier
        case "$track" in
            2) life_csv="5,35" ;;
            3) life_csv="5,20,35" ;;
            4) life_csv="5,15,35,50" ;;
        esac
        shot "${track}p_${oname}_lifecolor_mixed.png" \
            --screen ${track}p --track "$track" --orientation "$orient" \
            --color-mode 1 --starting-life 40 --life "$life_csv"
    done
done

# ============================================================
# 5. Selected player (no preview) — multiplayer × orientations
# ============================================================
for track in 2 3 4; do
    max_player=$((track - 1))
    for orient in 0 1 2; do
        orient_name=("absolute" "centric" "tabletop")
        oname=${orient_name[$orient]}
        for player in $(seq 0 $max_player); do
            shot "${track}p_${oname}_selected_p${player}.png" \
                --screen ${track}p --track "$track" --orientation "$orient" \
                --selected "$player"
        done
    done
done

# ============================================================
# 6. Random counters — multiplayer × orientations
# ============================================================
for track in 2 3 4; do
    for orient in 0 1 2; do
        orient_name=("absolute" "centric" "tabletop")
        oname=${orient_name[$orient]}
        shot "${track}p_${oname}_counters.png" \
            --screen ${track}p --track "$track" --orientation "$orient" \
            --random-counters
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
shot "damage_nonzero.png" --screen damage --enemy-damage 7,3,12

# ============================================================
# 10. Select screen with accumulated commander damage
# ============================================================
shot "select_with_damage.png" --screen select --enemy-damage 14,6,21

# ============================================================
# 11. Custom-life with non-default value
# ============================================================
shot "custom_life_123.png" --screen custom-life --starting-life 123

# ============================================================
# 12. Game-mode with non-default settings
# ============================================================
shot "game_mode_2p_20.png" --screen game-mode --players 2 --track 2 --starting-life 20
shot "game_mode_3p_30.png" --screen game-mode --players 3 --track 3 --starting-life 30

# ============================================================
# 13. Settings pages with different toggle states
# ============================================================
for dim in 0 1 2 3; do
    dim_name=("off" "15s" "30s" "60s")
    shot "settings_menu_dim_${dim_name[$dim]}.png" --screen settings-menu --auto-dim "$dim"
done

for cm in 0 1; do
    cm_name=("player" "life")
    for dt in 0 1 2 3; do
        dt_name=("never" "5s" "15s" "30s")
        for rot in 0 1 2; do
            rot_name=("absolute" "centric" "tabletop")
            shot "settings_more_cm${cm_name[$cm]}_ds${dt_name[$dt]}_${rot_name[$rot]}.png" \
                --screen settings-more --color-mode "$cm" --deselect "$dt" --orientation "$rot"
        done
    done
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
# 19. Intro screen
# ============================================================
shot "intro.png" --screen intro

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
SEC_LIFE=(); SEC_LIFECOLOR=(); SEC_SELECTED=(); SEC_COUNTERS=()
SEC_BRIGHT=(); SEC_COUNTER_EDIT=(); SEC_DAMAGE=(); SEC_SETTINGS=()
SEC_OTHER=()

for f in "${FILES[@]}"; do
    case "$f" in
        1p_preview_*)         SEC_1P_PREV+=("$f") ;;
        2p_*_preview_*)       SEC_2P_PREV+=("$f") ;;
        3p_*_preview_*)       SEC_3P_PREV+=("$f") ;;
        4p_*_preview_*)       SEC_4P_PREV+=("$f") ;;
        *_lifecolor_*)        SEC_LIFECOLOR+=("$f") ;;
        *_life[0-9]*)         SEC_LIFE+=("$f") ;;
        *_selected_*)         SEC_SELECTED+=("$f") ;;
        *_counters.png)       SEC_COUNTERS+=("$f") ;;
        brightness_*)         SEC_BRIGHT+=("$f") ;;
        counter_edit_*)       SEC_COUNTER_EDIT+=("$f") ;;
        damage_*|select_*|all_damage_*) SEC_DAMAGE+=("$f") ;;
        settings_*)           SEC_SETTINGS+=("$f") ;;
        *)                    SEC_OTHER+=("$f") ;;
    esac
done

write_section "1-Player Life Preview" "${SEC_1P_PREV[@]}"
write_section "2-Player Life Preview" "${SEC_2P_PREV[@]}"
write_section "3-Player Life Preview" "${SEC_3P_PREV[@]}"
write_section "4-Player Life Preview" "${SEC_4P_PREV[@]}"
write_section "Life Totals (Player Colors)" "${SEC_LIFE[@]}"
write_section "Life Totals (Life Colors)" "${SEC_LIFECOLOR[@]}"
write_section "Selected Player" "${SEC_SELECTED[@]}"
write_section "Player Counters" "${SEC_COUNTERS[@]}"
write_section "Brightness" "${SEC_BRIGHT[@]}"
write_section "Counter Edit" "${SEC_COUNTER_EDIT[@]}"
write_section "Damage / Select" "${SEC_DAMAGE[@]}"
write_section "Settings" "${SEC_SETTINGS[@]}"
write_section "Other" "${SEC_OTHER[@]}"

echo '</body></html>' >> "$INDEX"

echo ""
echo "Generated $COUNT screenshots in $OUT/"
echo "Index: $OUT/index.html"
