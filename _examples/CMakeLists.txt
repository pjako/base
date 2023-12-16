

app(triangle triangle.c)
target_link_libraries(triangle base os app rx)


app(sample_texture sample_texture.c)
shd_shader(sample_texture _examples/sample_texture.hlsl.h _examples/sample_texture.hlsl)
target_link_libraries(sample_texture base os app rx)

app(sample_cube sample_cube.c)
shd_shader(sample_cube _examples/sample_cube.hlsl.h _examples/sample_cube.hlsl)
target_link_libraries(sample_cube base os app rx)


app(sample_instancing sample_instancing.c)
shd_shader(sample_instancing _examples/sample_instancing.hlsl.h _examples/sample_instancing.hlsl)
target_link_libraries(sample_instancing base os app rx)