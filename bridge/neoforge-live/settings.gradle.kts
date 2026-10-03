pluginManagement {
    repositories {
        maven { url = uri("https://maven.neoforged.net/releases") }
        gradlePluginPortal()
        mavenCentral()
    }
}

plugins {
    id("org.gradle.toolchains.foojay-resolver-convention") version "0.9.0"
}

rootProject.name = "leaf-bootstrap-neoforge-live"
