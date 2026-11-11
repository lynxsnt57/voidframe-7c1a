/// blend_value(Value1, Value2, Mix) 
/// @arg Value1
/// @arg Value2
/// @arg Mix

function blend_value(Value1, Value2, mix) 
{
    return (1 - mix) * Value1 + mix * Value2
}
