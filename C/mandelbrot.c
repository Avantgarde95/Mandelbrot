#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <png.h> // libpng

typedef struct _pixel_t {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
} pixel_t;

typedef struct _bitmap_t {
    pixel_t* pixels;
    size_t width;
    size_t height;
} bitmap_t;

// ---------- png handling ----------
// (Most of the codes are from http://www.lemoda.net/c/write-png)

pixel_t* pixel_at(bitmap_t* bitmap, int x, int y) {
    return bitmap->pixels + bitmap->width*y + x;
}

int save_png(bitmap_t* bitmap, char* path) {
    FILE* fp;
    png_structp png_ptr = NULL;
    png_infop info_ptr = NULL;
    size_t x, y;
    png_byte** row_pointers = NULL;
    int status = -1;
    int pixel_size = 3;
    int depth = 8;

    fp = fopen(path, "wb");
    
    if (!fp) {
        return status;
    }

    png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING,
            NULL, NULL, NULL);

    if (png_ptr == NULL) {
        fclose(fp);
        return status;
    }

    info_ptr = png_create_info_struct(png_ptr);

    if (info_ptr == NULL) {
        png_destroy_write_struct(&png_ptr, &info_ptr);
        fclose(fp);
        return status;
    }

    if (setjmp(png_jmpbuf(png_ptr))) {
        png_destroy_write_struct(&png_ptr, &info_ptr);
        fclose(fp);
        return status;
    }

    png_set_IHDR(png_ptr,
            info_ptr,
            bitmap->width,
            bitmap->height,
            depth,
            PNG_COLOR_TYPE_RGB,
            PNG_INTERLACE_NONE,
            PNG_COMPRESSION_TYPE_DEFAULT,
            PNG_FILTER_TYPE_DEFAULT);

    row_pointers = png_malloc(png_ptr,
            bitmap->height * sizeof(png_byte*));

    for (y = 0; y < bitmap->height; y++) {
        png_byte* row = png_malloc(png_ptr,
                sizeof(uint8_t) * bitmap->width * pixel_size);
        row_pointers[y] = row;

        for (x = 0; x < bitmap->width; x++) {
            pixel_t* pixel = pixel_at(bitmap, x, y);
            *row++ = pixel->red;
            *row++ = pixel->green;
            *row++ = pixel->blue;
        }
    }

    png_init_io(png_ptr, fp);
    png_set_rows(png_ptr, info_ptr, row_pointers);
    png_write_png(png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);

    status = 0;

    for (y = 0; y < bitmap->height; y++) {
        png_free(png_ptr, row_pointers[y]);
    }

    png_free(png_ptr, row_pointers);

    png_destroy_write_struct(&png_ptr, &info_ptr);
    fclose(fp);
    return status;
}

// ---------- calculation ----------

pixel_t** generate_colormap(int size, pixel_t* start, pixel_t* end) {
    pixel_t** colormap;
    int dr, dg, db;
    int i;

    dr = (end->red - start->red) / ((float)size);
    dg = (end->green - start->green) / ((float)size);
    db = (end->blue - start->blue) / ((float)size);

    colormap = malloc(size * sizeof(*colormap));
    
    for (i = 0; i < size-1; i++) {
        colormap[i] = malloc(sizeof(*(colormap[i])));
        colormap[i]->red = (uint8_t)(start->red + i*dr);
        colormap[i]->green = (uint8_t)(start->green + i*dg);
        colormap[i]->blue = (uint8_t)(start->blue + i*db);
    }

    colormap[size-1] = malloc(sizeof(*(colormap[i])));
    colormap[size-1]->red = 0;
    colormap[size-1]->green = 0;
    colormap[size-1]->blue = 0;

    return colormap;
}

void destroy_colormap(pixel_t** colormap, int size) {
    int i;

    for (i = 0; i < size; i++) {
        free(colormap[i]);
    }

    free(colormap);
}

bitmap_t* generate_bitmap(int len_unit,
        int x_min, int x_max, int y_min, int y_max,
        pixel_t* color_start, pixel_t* color_end,
        int iter_max) {
    bitmap_t* bitmap;
    pixel_t** colormap;
    pixel_t *pixel, *color;
    float a, b, a_temp, b_temp;
    float x, y;
    int i, j, k;

    bitmap = malloc(sizeof(*bitmap));

    bitmap->width = len_unit * (x_max-x_min);
    bitmap->height = len_unit * (y_max-y_min);
    bitmap->pixels = malloc(
            bitmap->width * bitmap->height * sizeof(*(bitmap->pixels)));

    colormap = generate_colormap(iter_max, color_start, color_end);

    for (i = 0; i < bitmap->height; i++) {
        for (j = 0; j < bitmap->width; j++) {
            x = x_min + j/((float)len_unit);
            y = y_min + i/((float)len_unit);
            a = 0, b = 0, k = 0;

            while ((a*a+b*b < 2) && k < iter_max) {
                a_temp = a*a - b*b + x, b_temp = 2*a*b + y;

                if ((a == a_temp) && (b == b_temp)) {
                    k = iter_max;
                    break;
                }

                a = a_temp, b = b_temp;
                k++;
            }

            pixel = pixel_at(bitmap, j, i);
            color = colormap[k-1];

            pixel->red = color->red;
            pixel->green = color->green;
            pixel->blue = color->blue;
        }
    }
    
    destroy_colormap(colormap, iter_max);

    return bitmap;
}

void destroy_bitmap(bitmap_t* bitmap) {
    free(bitmap->pixels);
    free(bitmap);
}

// ---------- main ----------

int main(int argc, char** argv) {
    pixel_t color_start = {255, 50, 0};
    pixel_t color_end = {0, 0, 255};
    bitmap_t* bitmap;
    
    int len_unit;

    if (argc < 2) {
        len_unit = 1000;
    }
    else {
        len_unit = atoi(argv[1]);

        if (len_unit == 0) {
            len_unit = 1000;
        }
    }

    bitmap = generate_bitmap(
            len_unit, -2, 1, -1, 1, &color_start, &color_end, 100);

    if (save_png(bitmap, "out.png") != -1) {
        puts("Done!");
    }
    else {
        puts("Failed to save the file!");
    }

    destroy_bitmap(bitmap);

    return 0;
}
