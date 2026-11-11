/// render_low()
/// @desc Renders one complete low-quality frame. Viewport caching is owned by view_update_surface.

function render_low()
{
 var surf, finalsurf;
 render_surface[0] = surface_require(render_surface[0], render_width, render_height)
 surf = render_surface[0]
 render_alpha_hash = false
 render_alpha_hash_force = true
 render_light_tl = "Main"

 // A requested frame must replace the old scene before any gizmos are drawn.
 // Never use the shared redraw counter to reuse an already decorated target here.
 surface_set_target(surf)
 {
  draw_clear_alpha(c_black, 0)
  render_world_background()
  render_world_start()
  render_world_sky()
  render_world(render_lights ? e_render_mode.COLOR_FOG_LIGHTS : e_render_mode.COLOR_FOG)
  render_world_done()
  render_set_projection_ortho(0, 0, render_width, render_height, 0)
  gpu_set_blendmode_ext(bm_src_color, bm_one)
  if (render_background) draw_box(0, 0, render_width, render_height, false, c_black, 1)
  gpu_set_blendmode(bm_normal)
 }
 surface_reset_target()

 // Refresh the complete effect schedule for this frame, including camera-only effects.
 // A finished previous pass must not suppress post processing or target replacement.
 render_refresh_effects()
 finalsurf = render_post(surf)
 render_target = surface_require(render_target, render_width, render_height)
 surface_set_target(render_target)
 {
  draw_clear_alpha(c_black, 0)
  draw_surface_exists(finalsurf, 0, 0)
 }
 surface_reset_target()
 render_alpha_hash_force = false
}
