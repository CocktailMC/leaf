plugins {
    kotlin("multiplatform") version "2.0.21"
}

kotlin {
    linuxX64("native") {
        compilations.getByName("main") {
            cinterops {
                val leaf by creating {
                    defFile(project.file("leaf.def"))
                    includeDirs.allHeaders("../../abi/include")
                }
            }
        }
        binaries {
            sharedLib {
                baseName = "leaf_kn_sdk"
            }
        }
    }

    sourceSets {
        val nativeMain by getting {
            kotlin.srcDir("src")
        }
    }
}

repositories {
    mavenCentral()
}
