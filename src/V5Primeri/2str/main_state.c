#include <main_state.h>
#include <glad/glad.h>
#include <math.h>


#include <rafgl.h>

#include <game_constants.h>

static rafgl_raster_t doge;
static rafgl_raster_t upscaled_doge;
static rafgl_raster_t raster, raster2;
static rafgl_raster_t checker;

static rafgl_texture_t texture;


static int raster_width = RASTER_WIDTH, raster_height = RASTER_HEIGHT;

static char save_file[256];
int save_file_no = 0;

typedef struct _star_t
{
    int x, y, z;
} star_t;


#define STAR_MAX 500
#define STAR_Z_MAX 2000
star_t stars[STAR_MAX];


void init_stars(void)
{
    int i;
    for(i = 0; i < STAR_MAX; i++)
    {

        stars[i].z = randf() * STAR_Z_MAX;
        stars[i].x = randf() * STAR_Z_MAX - STAR_Z_MAX / 2;
        stars[i].y = randf() * STAR_Z_MAX - STAR_Z_MAX / 2;
    }
}

void update_stars(int speed)
{
    int i;
    for(i = 0; i < STAR_MAX; i++)
    {
        stars[i].z -= speed;
        if(stars[i].z <= 1)
        {
            stars[i].z += STAR_Z_MAX;
            stars[i].x = randf() * STAR_Z_MAX - STAR_Z_MAX / 2;
            stars[i].y = randf() * STAR_Z_MAX - STAR_Z_MAX / 2;
        }
    }
}

void render_stars(rafgl_raster_t *raster, int speed)
{
    int i, brightness;
    float sx0, sy0, sx1, sy1;

    for(i = 0; i < STAR_MAX; i++)
    {
        sx0 = raster_width / 2 + stars[i].x * (0.5 * raster_width / stars[i].z);
        sy0 = raster_height / 2 + stars[i].y * (0.5 * raster_height / stars[i].z);

        sx1 = raster_width / 2 + stars[i].x * (0.5 * raster_width / (stars[i].z + speed));
        sy1 = raster_height / 2 + stars[i].y * (0.5 * raster_height / (stars[i].z + speed));

        brightness = 255 - ((float)stars[i].z / STAR_Z_MAX) * 255.0f;

        rafgl_raster_draw_line(raster, sx0, sy0, sx1, sy1, rafgl_RGB(brightness, brightness, brightness));

    }

}


void main_state_init(GLFWwindow *window, void *args)
{
    /* inicijalizacija */
    /* raster init nam nije potreban ako radimo load from image */
    rafgl_raster_load_from_image(&doge, "res/images/doge.png");
    rafgl_raster_load_from_image(&checker, "res/images/checker32.png");

    rafgl_raster_init(&upscaled_doge, raster_width, raster_height);
    rafgl_raster_bilinear_upsample(&upscaled_doge, &doge);


    rafgl_raster_init(&raster, raster_width, raster_height);
    rafgl_raster_init(&raster2, raster_width, raster_height);

    init_stars();

    rafgl_texture_init(&texture);
}


int pressed;
float location = 0;
float selector = 0;


void main_state_update(GLFWwindow *window, float delta_time, rafgl_game_data_t *game_data, void *args)
{
    /* hendluj input */
    if(game_data->is_lmb_down && game_data->is_rmb_down)
    {
        pressed = 1;
        location = rafgl_clampf(game_data->mouse_pos_y, 0, raster_height - 1);
        selector = 1.0f * location / raster_height;
    }
    else
    {
        pressed = 0;
    }

    int star_speed = 10 + 90 * selector;

    update_stars(star_speed);
    /* izmeni raster */

    int x, y;

    float xn, yn;

    rafgl_pixel_rgb_t sampled, sampled2, resulting, resulting2;


    for(y = 0; y < raster_height; y++)
    {
        yn = 1.0f * y / raster_height;
        for(x = 0; x < raster_width; x++)
        {
            xn = 1.0f * x / raster_width;

            sampled = pixel_at_m(upscaled_doge, x, y);
            sampled2 = rafgl_point_sample(&doge, xn, yn);

            resulting = sampled;
            resulting2 = sampled2;

            resulting.rgba = rafgl_RGB(0, 0, 0);

            pixel_at_m(raster, x, y) = resulting;
            pixel_at_m(raster2, x, y) = resulting2;


            if(pressed && rafgl_distance1D(location, y) < 3 && x > raster_width - 15)
            {
                pixel_at_m(raster, x, y).rgba = rafgl_RGB(255, 0, 0);
            }

        }
    }

    render_stars(&raster, star_speed);
    /* shift + s snima raster */
    if(game_data->keys_pressed[RAFGL_KEY_S] && game_data->keys_down[RAFGL_KEY_LEFT_SHIFT])
    {
        sprintf(save_file, "save%d.png", save_file_no++);
        rafgl_raster_save_to_png(&raster, save_file);
    }



    /* update-uj teksturu*/
    if(!game_data->keys_down[RAFGL_KEY_SPACE])
        rafgl_texture_load_from_raster(&texture, &raster);
    else
        rafgl_texture_load_from_raster(&texture, &raster2);
}


void main_state_render(GLFWwindow *window, void *args)
{
    /* prikazi teksturu */
    rafgl_texture_show(&texture);
}


void main_state_cleanup(GLFWwindow *window, void *args)
{
    rafgl_raster_cleanup(&raster);
    rafgl_raster_cleanup(&raster2);
    rafgl_texture_cleanup(&texture);

}
