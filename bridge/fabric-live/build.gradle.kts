plugins {
    id("fabric-loom") version "1.9.2"
    java
}

group = "dev.leafmc"
version = "0.1.0"

base {
    archivesName.set("leaf-bootstrap-fabric-live")
}

repositories {
    mavenCentral()
    maven("https://maven.fabricmc.net/")
}

dependencies {
    minecraft("com.mojang:minecraft:1.21.1")
    mappings("net.fabricmc:yarn:1.21.1+build.3:v2")
    modImplementation("net.fabricmc:fabric-loader:0.16.14")
    modImplementation("net.fabricmc.fabric-api:fabric-api:0.116.0+1.21.1")
}

// LeafBridge uses FFM (final in JDK 22+). Loom remapper currently rejects
// classfile major>66 in some versions, so emit Java 22 bytecode.
java {
    toolchain {
        languageVersion.set(JavaLanguageVersion.of(25))
    }
    withSourcesJar()
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

tasks.processResources {
    duplicatesStrategy = DuplicatesStrategy.EXCLUDE
    inputs.property("version", version)
    filesMatching("fabric.mod.json") {
        expand("version" to project.version)
    }
}

tasks.withType<JavaCompile>().configureEach {
    options.release.set(22)
    options.encoding = "UTF-8"
}

// Copy the built native host into resources so Loom runServer can extract it.
val leafNative = rootProject.file("../../build/engine/bridge-host/libleaf_bridge.so")
tasks.named<ProcessResources>("processResources") {
    from(leafNative) {
        into("native/linux-x86_64")
    }
    onlyIf { leafNative.isFile }
    duplicatesStrategy = DuplicatesStrategy.INCLUDE
}

loom {
    runs {
        named("server") {
            vmArg("--enable-native-access=ALL-UNNAMED")
            vmArg("-Dleaf.leafmods.dir=${project.projectDir}/run/leafmods")
            vmArg("-Dleaf.minecraft.version=1.21.1")
            vmArg("-Dleaf.game.dir=${project.projectDir}/run")
            vmArg("-Dleaf.bridge.library=${leafNative.absolutePath}")
            System.getenv("LEAF_BRIDGE_LIBRARY")?.let {
                vmArg("-Dleaf.bridge.library=$it")
            }
        }
        named("client") {
            vmArg("--enable-native-access=ALL-UNNAMED")
            vmArg("-Dleaf.leafmods.dir=${project.projectDir}/run/leafmods")
            vmArg("-Dleaf.minecraft.version=1.21.1")
            vmArg("-Dleaf.game.dir=${project.projectDir}/run")
            vmArg("-Dleaf.bridge.library=${leafNative.absolutePath}")
            System.getenv("LEAF_BRIDGE_LIBRARY")?.let {
                vmArg("-Dleaf.bridge.library=$it")
            }
        }
    }
}
