plugins {
    id("java-library")
    id("net.neoforged.gradle.userdev") version "7.1.39"
}

version = property("mod_version") as String
group = property("mod_group_id") as String

base {
    archivesName.set(property("mod_id") as String)
}

java {
    // FFM needs 22+; NeoForge game still runs on JDK 22–25.
    toolchain {
        languageVersion.set(JavaLanguageVersion.of(25))
    }
}

repositories {
    mavenCentral()
}

sourceSets {
    main {
        java {
            srcDir("../common/src/main/java")
            exclude("dev/leafmc/bridge/smoke/**")
            exclude("dev/leafmc/bridge/harness/**")
        }
    }
}

dependencies {
    implementation("net.neoforged:neoforge:${property("neo_version")}")
}

runs {
    configureEach {
        systemProperty("forge.logging.markers", "REGISTRIES")
        systemProperty("forge.logging.console.level", "info")
        systemProperty("leaf.minecraft.version", "1.21.1")
        systemProperty("leaf.leafmods.dir", file("run/leafmods").absolutePath)
        systemProperty("leaf.game.dir", file("run").absolutePath)
        systemProperty(
            "leaf.bridge.library",
            rootProject.file("../../build/engine/bridge-host/libleaf_bridge.so").absolutePath,
        )
        jvmArgument("--enable-native-access=ALL-UNNAMED")
        workingDirectory(project.layout.projectDirectory.dir("run").dir(name))
        modSource(project.sourceSets.main.get())
    }
    named("server") {
        argument("--nogui")
    }
}

tasks.withType<JavaCompile>().configureEach {
    options.release.set(22)
    options.encoding = "UTF-8"
}

tasks.named<ProcessResources>("processResources") {
    val replaceProperties = mapOf(
        "minecraft_version" to project.property("minecraft_version"),
        "minecraft_version_range" to project.property("minecraft_version_range"),
        "neo_version" to project.property("neo_version"),
        "loader_version_range" to project.property("loader_version_range"),
        "mod_id" to project.property("mod_id"),
        "mod_name" to project.property("mod_name"),
        "mod_license" to project.property("mod_license"),
        "mod_version" to project.property("mod_version"),
    )
    inputs.properties(replaceProperties)
    filesMatching("META-INF/neoforge.mods.toml") {
        expand(replaceProperties)
    }
    val leafNative = rootProject.file("../../build/engine/bridge-host/libleaf_bridge.so")
    if (leafNative.isFile) {
        from(leafNative) {
            into("native/linux-x86_64")
        }
    }
}
