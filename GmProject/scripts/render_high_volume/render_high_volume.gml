/// Bounded volumes, using the current camera depth and current light shadow map.
function render_high_volume_begin()
{
    var volume_width = max(1, ceil(render_width * clamp(app.project_render_volume_resolution, 0.25, 1)))
    var volume_height = max(1, ceil(render_height * clamp(app.project_render_volume_resolution, 0.25, 1)))
    render_surface_volumetric = surface_require(render_surface_volumetric, volume_width, volume_height, true, true)
    surface_set_target(render_surface_volumetric)
    draw_clear_alpha(c_black, 0)
    surface_reset_target()
}

function render_high_volume_unshadowed()
{
    render_world_start()
    render_world_done()
    render_sun_direction = background_sun_direction
    render_light_color = background_sunlight_color_final
    render_light_strength = background_sunlight_strength
    if (render_light_strength > 0)
        render_high_volume_draw(1, null, render_surface_volumetric, false)
    var lights = array();
    with (obj_timeline)
        if ((type = e_tl_type.POINT_LIGHT || type = e_tl_type.SPOT_LIGHT) && !hide && value_inherit[e_value.VISIBLE] && !hq_hiding)
            lights = array_add(lights, id)
    for (var li = 0; li < array_length(lights); li++)
    {
        var light = lights[li];
        var lookat = point3D_add(light.world_pos, vec3(1, 0, 0));
        if (light.type = e_tl_type.SPOT_LIGHT)
            lookat = point3D_mul_matrix(point3D(0.0001, 1, 0), light.matrix)
        render_world_start_light(light.world_pos, lookat, vec3(0, 0, 0), light)
        render_world_start()
        render_world_done()
        render_high_volume_draw(light.type = e_tl_type.POINT_LIGHT ? 2 : 3, light, render_surface_volumetric, false)
    }
}

function render_high_volume_draw(pass, light, target, shadowed)
{
    var volumes = array();
    with (obj_timeline)
        if (type = e_tl_type.VOLUME && !hide && value_inherit[e_value.VISIBLE] && !hq_hiding)
            volumes = array_add(volumes, id)
    if (array_length(volumes) = 0) return 0
    surface_set_target(target)
    render_set_projection_ortho(0, 0, surface_get_width(target), surface_get_height(target), 0)
    gpu_set_ztestenable(false)
    if (pass = 0) gpu_set_blendmode(bm_normal)
    else gpu_set_blendmode(bm_add)
    render_shader_obj = shader_map[?shader_high_volume]
    for (var vi = 0; vi < array_length(volumes); vi++)
    {
        var volume = volumes[vi];
        with (render_shader_obj)
        {
            shader_set(shader)
            shader_high_volume_set(pass, volume, light, shadowed)
        }
        draw_blank(0, 0, surface_get_width(target), surface_get_height(target))
        with (render_shader_obj) shader_clear()
    }
    gpu_set_blendmode(bm_normal)
    surface_reset_target()
}

function shader_high_volume_set(pass, volume, light, shadowed)
{
    texture_set_stage(sampler_map[?"uSceneDepth"], surface_get_texture(render_surface_depth))
    gpu_set_texfilter_ext(sampler_map[?"uSceneDepth"], false)
    render_set_uniform("uInvProjection", matrix_inverse(proj_matrix))
    render_set_uniform("uInvView", matrix_inverse(view_matrix))
    render_set_uniform("uCameraViewProjection", view_proj_matrix)
    render_set_uniform("uVolumeInverse", matrix_inverse(volume.matrix))
    render_set_uniform_vec3("uCameraPosition", cam_from[X], cam_from[Y], cam_from[Z])
    render_set_uniform("uNear", depth_near)
    render_set_uniform("uFar", depth_far_custom)
    render_set_uniform("uMaxDistance", app.project_render_volume_far > 0 ? min(depth_far_custom, app.project_render_volume_far) : depth_far_custom)
    render_set_uniform("uMinDistance", max(0, app.project_render_volume_near))
    render_set_uniform("uJitter", clamp(app.project_render_volume_jitter, 0, 1))
    render_set_uniform_int("uShadowSamples", clamp(round(app.project_render_volume_shadow_samples), 1, 8))
    render_set_uniform("uShadowSoftness", max(0, app.project_render_volume_shadow_softness))
    render_set_uniform("uSunTexel", 1 / max(1, app.project_render_shadows_sun_buffer_size))
    if (pass = 2) render_set_uniform_vec2("uLocalTexel", 1 / max(1, 3 * app.project_render_shadows_point_buffer_size), 1 / max(1, 2 * app.project_render_shadows_point_buffer_size))
    else render_set_uniform_vec2("uLocalTexel", 1 / max(1, app.project_render_shadows_spot_buffer_size), 1 / max(1, app.project_render_shadows_spot_buffer_size))
    render_set_uniform_int("uPass", pass)
    render_set_uniform_int("uShadowEnabled", shadowed)
    render_set_uniform("uSampleIndex", render_sample_current)
    render_set_uniform_color("uFogColor", volume.value[e_value.LIGHT_COLOR], 1)
    render_set_uniform_color("uFogTopColor", volume.value[e_value.VOLUME_TOP_COLOR], 1)
    render_set_uniform_color("uEmissionColor", volume.value[e_value.VOLUME_EMISSION_COLOR], 1)
    render_set_uniform_int("uShape", volume.value[e_value.VOLUME_SHAPE])
    render_set_uniform_int("uPhase", volume.value[e_value.VOLUME_PHASE])
    render_set_uniform_int("uGradient", volume.value[e_value.VOLUME_GRADIENT])
    render_set_uniform_int("uNoiseWorld", volume.value[e_value.VOLUME_NOISE_WORLD])
    render_set_uniform("uEmission", volume.value[e_value.VOLUME_EMISSION])
    render_set_uniform("uNoiseDetail", volume.value[e_value.VOLUME_NOISE_DETAIL])
    render_set_uniform("uNoiseContrast", volume.value[e_value.VOLUME_NOISE_CONTRAST])
    render_set_uniform("uNoiseThreshold", volume.value[e_value.VOLUME_NOISE_THRESHOLD])
    render_set_uniform("uNoiseSoftness", volume.value[e_value.VOLUME_NOISE_SOFTNESS])
    render_set_uniform("uNoiseSeed", volume.value[e_value.VOLUME_NOISE_SEED])
    render_set_uniform_vec3("uWindDirection", volume.value[e_value.VOLUME_WIND_X], volume.value[e_value.VOLUME_WIND_Y], volume.value[e_value.VOLUME_WIND_Z])
    render_set_uniform("uWindSpeed", volume.value[e_value.VOLUME_WIND_SPEED])
    render_set_uniform("uWindInfluence", volume.value[e_value.VOLUME_WIND_INFLUENCE])
    render_set_uniform("uTurbulence", volume.value[e_value.VOLUME_TURBULENCE])
    render_set_uniform("uTurbulenceScale", volume.value[e_value.VOLUME_TURBULENCE_SCALE])
    render_set_uniform("uTime", app.timeline_marker / max(1, app.project_tempo))
    render_set_uniform("uDensity", volume.value[e_value.VOLUME_DENSITY])
    render_set_uniform("uScattering", volume.value[e_value.VOLUME_SCATTERING])
    render_set_uniform("uAbsorption", volume.value[e_value.VOLUME_ABSORPTION])
    render_set_uniform("uAnisotropy", volume.value[e_value.VOLUME_ANISOTROPY])
    render_set_uniform("uEdgeSoftness", volume.value[e_value.VOLUME_EDGE])
    render_set_uniform("uNoiseAmount", volume.value[e_value.VOLUME_NOISE_AMOUNT])
    render_set_uniform("uNoiseScale", volume.value[e_value.VOLUME_NOISE_SCALE])
    render_set_uniform_int("uSamples", clamp(round(volume.value[e_value.VOLUME_SAMPLES] * app.project_render_volume_quality), 8, 128))
    if (pass = 0)
    {
        render_set_uniform_color("uLightColor", app.background_ambient_color_final, 1)
        render_set_uniform("uLightStrength", 1)
    }
    else
    {
        render_set_uniform_color("uLightColor", render_light_color, 1)
        render_set_uniform("uLightStrength", render_light_strength)
        if (pass = 1)
        {
            render_set_uniform_vec3("uSunDirection", render_sun_direction[X], render_sun_direction[Y], render_sun_direction[Z])
            if (shadowed)
            {
                render_set_uniform("uSunMatrix0", render_cascades[0].matBias)
                render_set_uniform("uSunMatrix1", render_cascades[1].matBias)
                render_set_uniform("uSunMatrix2", render_cascades[2].matBias)
                render_set_uniform_vec3("uSunNear", render_cascades[0].near, render_cascades[1].near, render_cascades[2].near)
                render_set_uniform_vec3("uSunFar", render_cascades[0].far, render_cascades[1].far, render_cascades[2].far)
                render_set_uniform_vec3("uCascadeEnd", render_cascades[0].clipEndDepth, render_cascades[1].clipEndDepth, render_cascades[2].clipEndDepth)
                texture_set_stage(sampler_map[?"uSun0"], surface_get_texture(render_surface_sun_buffer[0]))
                texture_set_stage(sampler_map[?"uSun1"], surface_get_texture(render_surface_sun_buffer[1]))
                texture_set_stage(sampler_map[?"uSun2"], surface_get_texture(render_surface_sun_buffer[2]))
                gpu_set_texfilter_ext(sampler_map[?"uSun0"], false)
                gpu_set_texfilter_ext(sampler_map[?"uSun1"], false)
                gpu_set_texfilter_ext(sampler_map[?"uSun2"], false)
            }
        }
        else
        {
            render_set_uniform_vec3("uLightPosition", render_light_from[X], render_light_from[Y], render_light_from[Z])
            render_set_uniform_vec3("uShadowPosition", render_shadow_from[X], render_shadow_from[Y], render_shadow_from[Z])
            render_set_uniform("uLightNear", render_light_near)
            render_set_uniform("uLightFar", render_light_far)
            render_set_uniform("uLightFade", render_light_fade_size)
            if (pass = 2 && shadowed)
                texture_set_stage(sampler_map[?"uLocalShadow"], surface_get_texture(render_surface_point_atlas_buffer))
            if (shadowed) gpu_set_texfilter_ext(sampler_map[?"uLocalShadow"], false)
            if (pass = 3)
            {
                if (shadowed) texture_set_stage(sampler_map[?"uLocalShadow"], surface_get_texture(render_surface_spot_buffer))
                render_set_uniform("uSpotMatrix", render_spot_matrix)
                render_set_uniform("uSpotShadowMatrix", render_shadow_matrix)
                render_set_uniform("uSpotSharpness", render_light_spot_sharpness)
                render_set_uniform_int("uGoboMode", render_gobo_mode)
                render_set_uniform_int("uGoboTiling", render_gobo_tiling)
                render_set_uniform_int("uGoboInvert", render_gobo_invert)
                render_set_uniform_vec2("uGoboOffset", render_gobo_offset[X], render_gobo_offset[Y])
                render_set_uniform_vec2("uGoboRepeat", render_gobo_repeat[X], render_gobo_repeat[Y])
                render_set_uniform("uGoboRotation", render_gobo_rotation)
                render_set_uniform("uGoboIntensity", render_gobo_intensity)
                render_set_uniform("uGoboFeather", render_gobo_feather)
                render_set_uniform("uGoboGamma", render_gamma)
                texture_set_stage(sampler_map[?"uLightGobo"], sprite_get_texture(render_gobo_sprite, 0))
                gpu_set_texfilter_ext(sampler_map[?"uLightGobo"], true)
                gpu_set_texrepeat_ext(sampler_map[?"uLightGobo"], render_gobo_tiling)
            }
        }
    }
}
