package dev.leafmc.bridge;

import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;

/**
 * Optional {@code leaf.json} next to the game root:
 * <pre>
 * {
 *   "minecraftVersion": "1.21.1",
 *   "leafmodsDir": "leafmods",
 *   "autoLoadMods": true
 * }
 * </pre>
 * System properties always win over the file.
 */
public final class LeafConfigFile {
    private LeafConfigFile() {}

    public static void applyFileDefaults(LeafBridge.Config cfg) {
        Path game = cfg.gameDir != null && !cfg.gameDir.isBlank()
                ? Path.of(cfg.gameDir)
                : Path.of(".");
        Path file = game.resolve("leaf.json");
        if (!Files.isRegularFile(file)) {
            writeDefaultsIfAbsent(game);
            return;
        }
        try {
            String text = Files.readString(file, StandardCharsets.UTF_8);
            String mc = extractString(text, "minecraftVersion");
            String mods = extractString(text, "leafmodsDir");
            Boolean auto = extractBoolean(text, "autoLoadMods");
            if (mc != null && System.getProperty("leaf.minecraft.version") == null) {
                cfg.minecraftVersion = mc;
            }
            if (mods != null && System.getProperty("leaf.leafmods.dir") == null) {
                cfg.leafmodsDir = mods;
            }
            if (auto != null && System.getProperty("leaf.autoload") == null) {
                cfg.autoLoadMods = auto;
            }
        } catch (IOException ignored) {
            // optional file
        }
    }

    /** Write a starter leaf.json when missing (does not overwrite). */
    public static void writeDefaultsIfAbsent(Path gameDir) {
        if (gameDir == null) {
            return;
        }
        Path file = gameDir.resolve("leaf.json");
        if (Files.exists(file)) {
            return;
        }
        try {
            Files.createDirectories(gameDir);
            String body = """
                    {
                      "minecraftVersion": "1.21.1",
                      "leafmodsDir": "leafmods",
                      "autoLoadMods": true
                    }
                    """;
            Files.writeString(file, body, StandardCharsets.UTF_8);
        } catch (IOException ignored) {
            // best-effort
        }
    }

    // Tiny JSON subset extractor — avoids pulling a JSON library into the bridge.
    static String extractString(String json, String key) {
        String pattern = "\"" + key + "\"";
        int i = json.indexOf(pattern);
        if (i < 0) {
            return null;
        }
        int colon = json.indexOf(':', i + pattern.length());
        if (colon < 0) {
            return null;
        }
        int q1 = json.indexOf('"', colon + 1);
        if (q1 < 0) {
            return null;
        }
        int q2 = json.indexOf('"', q1 + 1);
        if (q2 < 0) {
            return null;
        }
        return json.substring(q1 + 1, q2);
    }

    static Boolean extractBoolean(String json, String key) {
        String pattern = "\"" + key + "\"";
        int i = json.indexOf(pattern);
        if (i < 0) {
            return null;
        }
        int colon = json.indexOf(':', i + pattern.length());
        if (colon < 0) {
            return null;
        }
        String rest = json.substring(colon + 1).trim();
        if (rest.startsWith("true")) {
            return true;
        }
        if (rest.startsWith("false")) {
            return false;
        }
        return null;
    }
}
