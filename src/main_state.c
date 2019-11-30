#include <main_state.h>
#include <glad/glad.h>
#include <math.h>


#include <rafgl.h>

#include <game_constants.h>

#define climbing_in_pos(x,xn,y) (animation_climbing || ((x < ladder_pos_x + ladder_width) && ((xn) > ladder_pos_x) && (y <= ladder_pos_y + ladder_height)))
#define walking_in_pos(y) (!animation_climbing || ((y) < platform_pos_y) || ((y) == RASTER_HEIGHT))

static rafgl_raster_t doge;
static rafgl_raster_t upscaled_doge;
static rafgl_raster_t raster, raster2;
static rafgl_raster_t checker;
static rafgl_raster_t brick_tile, wall_tile;
static rafgl_raster_t smallhero, bighero;
static rafgl_texture_t texture;

static rafgl_spritesheet_t hero;


static int raster_width = RASTER_WIDTH, raster_height = RASTER_HEIGHT;

static char save_file[256];
int save_file_no = 0;

int hero_pos_x = 0;
int hero_pos_y = 0;

int ladder_pos_x = 0;
int ladder_pos_y = 0;
int ladder_height = 0;
int ladder_width = 50;

int spell_pos_x = 0;
int spell_pos_y = 0;
int spell_speed = 400;
int spell_direction = -1;

int platform_pos_x = 0;
int platform_pos_y = 0;
int platform_len = 0;
int platform_height = 0;

int tail_pos_x = 0;
int tail_pos_y = 0;
int tail_max_len = 150;

int small_frame_width, small_frame_height;

int gravity = 10;
int hero_weight = 80;

void main_state_init(GLFWwindow *window, void *args)
{
    /* inicijalizacija */
    /* raster init nam nije potreban ako radimo load from image */
    rafgl_raster_load_from_image(&doge, "res/images/doge.png");
    rafgl_raster_load_from_image(&checker, "res/images/checker32.png");
    rafgl_raster_load_from_image(&brick_tile, "res/tiles/bricks.png");
    rafgl_raster_load_from_image(&wall_tile, "res/tiles/stone.png");

    rafgl_raster_init(&upscaled_doge, raster_width, raster_height);
    rafgl_raster_bilinear_upsample(&upscaled_doge, &doge);

    rafgl_raster_init(&raster, raster_width, raster_height);
    rafgl_raster_init(&raster2, raster_width, raster_height);

    rafgl_spritesheet_init(&hero, "res/images/character.png", 10, 4);

    rafgl_raster_init(&bighero, hero.sheet.width * 2, hero.sheet.height * 2);

    smallhero = hero.sheet;
    small_frame_height = hero.frame_height;
    small_frame_width = hero.frame_width;
    rafgl_raster_bilinear_upsample(&bighero, &smallhero);
    
    hero_pos_x -= hero.frame_width / 2;
    hero_pos_y -= hero.frame_height;

    hero_pos_x += RASTER_WIDTH / 2;
    hero_pos_y += RASTER_HEIGHT;

    rafgl_texture_init(&texture);
}


int pressed;
float location = 0;
float selector = 0;

int animation_big = 1;
int animation_falling = 0;
int animation_running = 0;
int animation_climbing = 0;
int animation_frame = 0;
int direction = 0;


int hero_speed = 150;

int hover_frames = 0;

void draw_platform(int x0, int y0, int lx, int ly){
    int x, y;

    platform_pos_x = x0;
    platform_pos_y = y0;
    platform_len = lx;
    platform_height = ly;

    for(y = y0; y < y0 + ly; y++){
        for(x = x0; x < x0 + lx; x++){
            pixel_at_m(raster, x, y).rgba = pixel_at_m(brick_tile, x % brick_tile.width, y % brick_tile.height).rgba;
        }
    }
}

void draw_ladder(int x0, int y0, int l, int factor) {
    int x, y;
    ladder_pos_x = x0;
    ladder_pos_y = y0;
    ladder_height = l;

    for(y = y0; y < y0 + l; y++)
    {
        for(x = x0; x < raster_width; x++)
        {
            if(x > x0 && x < x0 + 50){
                if (x > x0 && x < x0 + 10) {
                    if(x < x0 + 3){
                        pixel_at_m(raster, x, y).rgba = rafgl_RGB(55,55,55);
                    } else if (x < x0 + 8) {
                        pixel_at_m(raster, x, y).rgba = rafgl_RGB(255,255,255);
                    } else {
                        pixel_at_m(raster, x, y).rgba = rafgl_RGB(55,55,55);
                    }
                }
                else if(x < x0 + 51 && x > x0 + 39){
                    if(x < x0 + 42){
                        pixel_at_m(raster, x, y).rgba = rafgl_RGB(55,55,55);
                    } else if (x < x0 + 48) {
                        pixel_at_m(raster, x, y).rgba = rafgl_RGB(255,255,255);
                    } else {
                        pixel_at_m(raster, x, y).rgba = rafgl_RGB(55,55,55);
                    }
                } 
                if(y % factor < 8){
                    if(x >= x0 + 10 && x <= x0 + 39){
                        if(y % factor < 2){
                            pixel_at_m(raster, x, y).rgba = rafgl_RGB(55,55,55);
                        } else if(y % factor < 6) {   
                            pixel_at_m(raster, x, y).rgba = rafgl_RGB(255,255,255);
                        } else {
                            pixel_at_m(raster, x, y).rgba = rafgl_RGB(55,55,55);
                        }  
                    }
                }
            }

        }
    }
}

void draw_spell_animation(int h, int k, int radius){
    int x, y;

   //float rafgl_distance2D(float x1, float y1, float x2, float y2);
    rafgl_pixel_rgb_t blue_pix, white_pix;
    blue_pix.rgba = rafgl_RGB(137,207,240);
    white_pix.rgba = rafgl_RGB(255,255,255);

    for(y = k - radius; y <= k + radius; y++){
        for(x = h - radius; x <= h + radius; x++){
            if(rafgl_distance2D(x, y, h, k) <= radius){
                if((x <= 0) || x >= RASTER_WIDTH) continue;
                float scale =  1.0f - rafgl_distance2D(x, y, h, k) / radius;
                pixel_at_m(raster, x, y).rgba = rafgl_lerppix(blue_pix, white_pix, scale).rgba;
            }
        }
    }
}

void main_state_update(GLFWwindow *window, float delta_time, rafgl_game_data_t *game_data, void *args)
{
    /* hendluj input */
    int hover_val = 5;
    animation_running = 1;
    if((hero_pos_x + hero.frame_width < platform_pos_x || hero_pos_x > platform_pos_x + platform_len) && hero_pos_y + hero.frame_height < RASTER_HEIGHT)
        animation_falling = 1;
    if(animation_falling) {
        direction = 0;
        hero_pos_y = hero_pos_y + delta_time * gravity * hero_weight;
        if(hero_pos_y + hero.frame_height >= RASTER_HEIGHT)
            animation_falling = 0;
    }
    
    if(!animation_falling && climbing_in_pos(hero_pos_x,hero_pos_x + hero.frame_width, hero_pos_y) && game_data->keys_down[RAFGL_KEY_W])
    {   
        animation_climbing = 1;
        hero_pos_y = hero_pos_y - hero_speed * delta_time;
        direction = 2;
    }
    else if(!animation_falling && climbing_in_pos(hero_pos_x,hero_pos_x + hero.frame_width, hero_pos_y) && game_data->keys_down[RAFGL_KEY_S])
    {   
        animation_climbing = 1;
        hero_pos_y = hero_pos_y + hero_speed * delta_time;
        direction = 0;
    }
    else if(walking_in_pos(hero_pos_y + hero.frame_height) && game_data->keys_down[RAFGL_KEY_A])
    {
        if(hero_pos_y + hero.frame_height < platform_pos_y) hero_pos_y = platform_pos_y - hero.frame_height;
        animation_climbing = 0;
        hero_pos_x = hero_pos_x - hero_speed * delta_time;
        direction = 1;
    }
    else if(walking_in_pos(hero_pos_y + hero.frame_height) && game_data->keys_down[RAFGL_KEY_D])
    {
        if(hero_pos_y + hero.frame_height < 100) hero_pos_y = 100 - hero.frame_height;
        animation_climbing = 0;
        hero_pos_x = hero_pos_x + hero_speed * delta_time;
        direction = 3;
    }
    else
    {
        if(animation_climbing) direction = 2;
        else direction = 0;
        animation_frame = 0;
        animation_running = 0;
    }


    if(animation_running)
    {
        if(hover_frames == 0)
        {
            animation_frame = (animation_frame + 1) % 10;
            hover_frames = hover_val;
        }
        else
        {
            hover_frames--;
        }

    }

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

            resulting.rgba = pixel_at_m(wall_tile, x % wall_tile.width, y % wall_tile.height).rgba;

            pixel_at_m(raster, x, y) = resulting;
            pixel_at_m(raster2, x, y) = resulting2;


            if(pressed && rafgl_distance1D(location, y) < 3 && x > raster_width - 15)
            {
                pixel_at_m(raster, x, y).rgba = rafgl_RGB(0, 0, 0);
            }
        }
    }


    //void rafgl_raster_draw_line(rafgl_raster_t *raster, int x0, int y0, int x1, int y1, uint32_t colour);

    int hero_x, hero_y;

    if(hero_pos_x + hero.frame_width > raster.width){
        hero_x = raster.width - hero.frame_width;
        hero_pos_x = raster.width - hero.frame_width;
    }
    else if(hero_pos_x < 0){
        hero_x = 0;
        hero_pos_x = 0;
    }
    else
        hero_x = hero_pos_x;

    if(hero_pos_y + hero.frame_height > raster.height){
        hero_y = raster.height - hero.frame_height;
        hero_pos_y = raster.height - hero.frame_height;
    }
    else if(hero_pos_y < 0){
        hero_y = 0;
        hero_pos_y = 0;
    }
    else
        hero_y = hero_pos_y;

    draw_ladder(280, 60, 460, 40);
    
    if(game_data->keys_pressed[RAFGL_KEY_G])
        animation_big = !animation_big;
    
    if(animation_big){
        hero.sheet = bighero;
        hero.frame_height = small_frame_height * 2;
        hero.frame_width = small_frame_width * 2;
        rafgl_raster_draw_spritesheet(&raster, &hero, animation_frame, direction, hero_x, hero_y);
    } else {
        hero.sheet = smallhero;
        hero.frame_height = small_frame_height;
        hero.frame_width = small_frame_width;
        if((hero_pos_y + hero.frame_height != RASTER_HEIGHT || hero_pos_y + hero.frame_height != platform_pos_y) && !animation_climbing)
            animation_falling = 1;

        rafgl_raster_draw_spritesheet(&raster, &hero, animation_frame, direction, hero_x, hero_y);
    }

    draw_platform(200, 100, 300, 60);

    if((spell_direction < 0) && game_data -> keys_pressed[RAFGL_KEY_E]){
        int radius = 25;
        spell_direction = 2;
        spell_pos_x = hero_pos_x + hero.frame_width  + radius + 5;
        spell_pos_y = hero_pos_y + hero.frame_height / 2;

        draw_spell_animation(spell_pos_x, spell_pos_y, 25);
    } else if ((spell_direction < 0) && game_data -> keys_pressed[RAFGL_KEY_Q]) {
        int radius = 25;
        spell_direction = 1;
        spell_pos_x = hero_pos_x - radius - 5;
        spell_pos_y = hero_pos_y + hero.frame_height / 2;
        draw_spell_animation(spell_pos_x, spell_pos_y, 25);
    }

    if(spell_direction > 0 && spell_pos_x + 25 > 0 && (spell_pos_x - 25 < RASTER_WIDTH)){
        if(spell_direction == 2)
            spell_pos_x = spell_pos_x + delta_time * spell_speed;
        else if(spell_direction == 1)
            spell_pos_x = spell_pos_x - delta_time * spell_speed;

        draw_spell_animation(spell_pos_x, spell_pos_y, 25);
    } else {
        spell_direction = -1;
        spell_pos_x = -1;
        spell_pos_y = -1;
    }


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
