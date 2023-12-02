glsl_flags="-Werror -O -DGLSL=1 -std=450 --target-env=vulkan1.3 --target-spv=spv1.6 -I src"

python compile_glsl.py --out-dir compiled_shaders --flags "$glsl_flags" src/shaders/*.{glsl,comp} || exit 1
python compile_glsl.py --out-dir compiled_shaders/compute --flags "$glsl_flags" --shader-stage=comp src/shaders/compute/*.* || exit 1
