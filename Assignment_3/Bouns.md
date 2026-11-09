# Bonus Exercises – Implementation Notes

I implemented both bonus exercises: Depth of Field and Refraction.

## Bonus 1: Depth of Field

To test depth of field, set `enable_dof = true` in `src/main.cpp`.

I tested different camera aperture values (0.02, 0.05, and 0.2) to compare how the amount of blur changes.

The implementation takes 64 samples per pixel. Each ray starts at a slightly different position on a disk around the camera and aims at the same focal point.

Objects near the focal plane remain sharp, while objects farther away become blurred. Increasing the aperture produces more noticeable blur.

## Bonus 2: Refraction

To test refraction, set `enable_refraction = true`.

The implementation uses Snell's law to calculate the direction of refracted rays. The refraction index is set to 1.5 by default.

It also handles total internal reflection, where no refracted ray is generated.

## Testing and Comparison

During development, I temporarily saved rendered images using different settings in a separate `images/` folder. This allowed me to compare aperture values, refraction, and Perlin noise interpolation results.

The temporary images folder is not included in the final submission. The default settings in `src/main.cpp` are restored to render the main assignment exercises (0–4).