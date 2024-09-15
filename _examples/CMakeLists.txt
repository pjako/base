app(triangle triangle.c)
target_link_libraries(triangle base os app rx)

app(sample_texture sample_texture.c)
shd_shader(sample_texture sample_texture.hlsl.h sample_texture.hlsl)
target_link_libraries(sample_texture base os app rx)

app(sample_cube sample_cube.c)
shd_shader(sample_cube sample_cube.hlsl.h sample_cube.hlsl)
target_link_libraries(sample_cube base os app rx)

app(sample_instancing sample_instancing.c)
shd_shader(sample_instancing sample_instancing.hlsl.h sample_instancing.hlsl)
target_link_libraries(sample_instancing base os app rx)

app(sample_shadows sample_shadows.c)
shd_shader(sample_shadows sample_shadows.hlsl.h sample_shadows.hlsl)
target_link_libraries(sample_instancing base os app rx)

app(sample_sdfRect sample_sdfRect.c)
shd_shader(sample_sdfRect sample_sdfRect.hlsl.h sample_sdfRect.hlsl)
target_link_libraries(sample_sdfRect base os app rx)