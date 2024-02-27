if (Test-Path resources/shaders)
{
    Remove-Item resources/shaders -Recurse -Force -Confirm:$false
}
mkdir -p resources/shaders
glslc development_resources/shaders/shader.vert -o development_resources/shaders/shader.vert.spv
glslc development_resources/shaders/shader.frag -o development_resources/shaders/shader.frag.spv
spirv-link development_resources/shaders/shader.vert.spv development_resources/shaders/shader.frag.spv -o resources/shaders/shader.spv
Remove-Item development_resources/shaders/shader.vert.spv
Remove-Item development_resources/shaders/shader.frag.spv