dependencies {
    implementation(project(":common"))
    implementation(project(":fabric"))
    implementation(project(":forge"))
    implementation(project(":neoforge"))
    compileOnly(project(":stubs"))
}

tasks.jar {
    archiveBaseName.set("leaf-multi-loader-harness")
}
