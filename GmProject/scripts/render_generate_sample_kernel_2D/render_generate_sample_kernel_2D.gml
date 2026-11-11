/// render_generate_sample_kernel_2D(samples)
function render_generate_sample_kernel_2D(samples)
{
    var arr
    
    for (var i = 0; i < samples; i++)
    {
        // Uniform angle
        var angle = random(2 * 3.14159265);
        
        // Quadratic distribution (more samples near center)
        var t = i / samples;
        var radius = lerp(0.01, 1.0, t * t);
        
        // Polar → Cartesian
        var xv = cos(angle) * radius;
        var yv = sin(angle) * radius;
        
		if (abs(xv) < 0.001)
			xv = 0.001
        
		if (abs(yv) < 0.001)
			yv = 0.001
		
        arr[i * 2]     = xv;
        arr[i * 2 + 1] = yv;
    }
    
    return arr;
}