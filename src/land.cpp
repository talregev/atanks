#include "land.h"
#include "files.h"

#ifdef THREADS
#include <pthread.h>
#endif

// Define how the land will look.

void generate_land(GLOBALDATA *global, ENVIRONMENT *env, BITMAP *land, int xoffset, int yoffset, int heightx)
{
    const int land_height = heightx * 5 / 6;
    double smoothness = 100;
    int octaves = 8;
    double lambda = 0.25;
    double * depthStrip[2];
    //Ignore updates from other threads by using a temporary variable.
    int curland_temp=global->get_curland();

    depthStrip[0] = (double *) calloc(global->screenHeight, sizeof(double));
    depthStrip[1] = (double *) calloc(global->screenHeight, sizeof(double));

    if (!depthStrip[0] || !depthStrip[1])
        std::cerr << "ERROR: Unable to allocate " << (global->screenHeight * 2) << " bytes in generate_land() !!!" << std::endl;

    int landType = (env->landType == LANDTYPE_RANDOM)? (rand() % LANDTYPE_PLAIN) + 1 : (int) env->landType;

    switch (landType)
    {
    case LANDTYPE_MOUNTAINS:
        smoothness = 200;
        octaves = 8;
        lambda = 0.65;
        break;
    case LANDTYPE_CANYONS:
        smoothness = 50;
        octaves = 8;
        lambda = 0.25;
        break;
    case LANDTYPE_VALLEYS:
        smoothness = 200;
        octaves = 8;
        lambda = 0.25;
        break;
    case LANDTYPE_HILLS:
        smoothness = 600;
        octaves = 6;
        lambda = 0.40;
        break;
    case LANDTYPE_FOOTHILLS:
        smoothness = 1200;
        octaves = 3;
        lambda = 0.25;
        break;
    case LANDTYPE_PLAIN:
        smoothness = 4000;
        octaves = 2;
        lambda = 0.2;
        break;
    case LANDTYPE_NONE:
        break;
    default:
        break;
    }

    if (global->detailedLandscape)
        memset(depthStrip[1], 0, global->screenHeight * sizeof (double));

    for (int x = 0; x < global->screenWidth; x++)
    {
        int depth = 0;
        if (landType == LANDTYPE_NONE)
            env->height[x] = 1;
        else
            env->height[x] = ((perlin2DPoint(1.0, smoothness, xoffset + x, yoffset, lambda, octaves) + 1) / 2);

        if (global->detailedLandscape)
        {
            memcpy(depthStrip[0], depthStrip[1], global->screenHeight * sizeof(double));
            for (depth = 1; depth < global->screenHeight; depth++)
            {
                depthStrip[1][depth] = ((perlin2DPoint(1.0, smoothness, xoffset + x, yoffset + depth, lambda, octaves) + 1) / 2 * land_height - (global->screenHeight - depth));
                if (depthStrip[1][depth] > env->height[x] * land_height)
                    depthStrip[1][depth] = env->height[x] * land_height;
            }
            depthStrip[1][0] = 0;
            depth = 1;
        }

        if (landType == LANDTYPE_NONE)
            env->height[x] = 1;
        else
            env->height[x] *= land_height;

        for (int y = 0; y <= env->height[x]; y++)
        {
            double offset = 0;
            int color = 0;
            double shade = 0;

            if (global->detailedLandscape)
            {
                double bot; // top,
                double minBot, maxTop, btdiff, i;
                double a1, a2, angle;
                while ((depthStrip[1][depth] <= y) && (depth < global->screenHeight))
                    depth++;
                bot = (depthStrip[0][depth - 1] + depthStrip[1][depth - 1]) / 2;
                // top = (depthStrip[0][depth] + depthStrip[1][depth]) / 2;
                minBot = MIN (depthStrip[0][depth - 1], depthStrip[1][depth - 1]);
                maxTop = MAX (depthStrip[0][depth], depthStrip[1][depth]);
                btdiff = maxTop - minBot;
                i = (y - bot) / btdiff;
                a1 = atan2(depthStrip[0][depth - 1] - depthStrip[1][depth - 1], 1.0) * 180 / PI + 180;
                a2 = atan2(depthStrip[0][depth] - depthStrip[1][depth], 1.0) * 180 / PI + 180;

                angle = interpolate(a1, a2, i);
                shade = global->slope[(int)angle][0];
            }

            if (global->ditherGradients)
                offset += rand() % 10 - 5;

            if (global->detailedLandscape)
                offset += (global->screenHeight - depth) * 0.5;

            while (y + offset < 0)
                offset /= 2;
            while (y + offset > global->screenHeight)
                offset /= 2;

            color = gradientColorPoint(land_gradients[curland_temp], env->height[x], y + offset);
            if (global->detailedLandscape)
            {
                float h, s, v;
                int r, g, b;

                r = getr(color);
                g = getg(color);
                b = getb(color);
                rgb_to_hsv(r, g, b, &h, &s, &v);
                shade += (double) (rand() % 1000 - 500) * (1.0/10000);
                if (shade < 0)
                    v += v * shade * 0.5;
                else
                    v += (1 - v) * shade * 0.5;
                hsv_to_rgb(h, s, v, &r, &g, &b);
                color = makecol(r, g, b);
            }

#ifdef THREADS
            solid_mode();
#endif
            putpixel(land, x, global->screenHeight - y, color);
#ifdef THREADS
            drawing_mode(global->env->current_drawing_mode, NULL, 0, 0);
#endif
        }
    }

    free(depthStrip[0]);
    free(depthStrip[1]);
}

#ifdef THREADS
// This function should be called as a separate thread. Its sole purpose
// is to generate terrain images in the background. Then pass them
// to the main program.
void *Generate_Land_In_Background(void *new_env)
{
    ENVIRONMENT *env = (ENVIRONMENT *) new_env;
    GLOBALDATA *global = env->Get_Globaldata();
    BITMAP *land = NULL;
    int xoffset;

    while (global->get_command() != GLOBAL_COMMAND_QUIT)
    {
        if (!env->get_waiting_terrain())
        {
            // The sky thread runs around the same time. We will wait a few seconds
            LINUX_DREAMLAND;
            land = create_bitmap(global->screenWidth, global->screenHeight);
            if (!land)
                printf("Memory error while creating land in background.\n");
            else
            {
                clear_to_color(land, PINK);
                xoffset = rand();
                generate_land(global, env, land, xoffset, 0, global->screenHeight);
            }
            env->lock(env->waiting_terrain_lock);
            env->waiting_terrain = land;
            env->unlock(env->waiting_terrain_lock);
        }
        LINUX_SLEEP;        // avoid taxing the CPU
    }        // while still running program

    pthread_exit(NULL);
    return NULL;         // keeps the compiler happy
}
#endif
