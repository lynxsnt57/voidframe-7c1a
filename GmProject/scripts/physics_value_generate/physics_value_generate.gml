/// physics_value_generate(prev_value, prev_velocity, current_value, stiffness, damping)
/// @arg prev_value
/// @arg prev_velocity
/// @arg current_value
/// @arg stiffness
/// @arg damping
/// @desc Return Vec2(Value, Velocity)


function physics_value_generate(prev_value, prev_velocity, current_value, stiffness, damping)
{
    prev_velocity += (current_value - prev_value) * stiffness;
    prev_velocity *= damping;
    prev_value += prev_velocity;

    return vec2(prev_value, prev_velocity);
}