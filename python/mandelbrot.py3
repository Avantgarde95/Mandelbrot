import sys
from PIL import Image

def generate_colormap(size, start = (255, 255, 255), end = (0, 0, 0)):
    rs, gs, bs = start
    re, ge, be = end

    dr = (re-rs) / float(size)
    dg = (ge-gs) / float(size)
    db = (be-bs) / float(size)

    colormap = []

    for i in range(size):
        r, g, b = int(rs + i*dr), int(gs + i*dg), int(bs + i*db)
        colormap.append(chr(r) + chr(g) + chr(b))

    return colormap

def generate_image(len_unit = 200,
                   range_x = (-2, 1),
                   range_y = (-1, 1),
                   range_color = ((255, 50, 0), (0, 0, 255)),
                   iter_max = 50):
    x_min, x_max = range_x
    y_min, y_max = range_y
    w, h = len_unit * (x_max-x_min), len_unit * (y_max-y_min)

    data = [None] * (w*h)
    color = generate_colormap(size = iter_max - 1,
                              start = range_color[0],
                              end = range_color[1])
    color += ['\x00\x00\x00'] # internal : black

    # main calculation
    for i in range(h):
        for j in range(w):
            x, y = x_min + j/float(len_unit), y_min + i/float(len_unit)
            a, b = 0, 0
            k = 0

            while a*a+b*b < 2 and k < iter_max:
                a_temp, b_temp = a*a - b*b + x, 2*a*b + y

                # period checking
                if a == a_temp and b == b_temp:
                    k = iter_max
                    break

                a, b = a_temp, b_temp
                k += 1

            data[w*i+j] = color[k-1]

    return Image.frombytes(mode = 'RGB',
                           size = (w, h),
                           data = bytes(ord(c) for c in ''.join(data)))

if __name__ == '__main__':
    if len(sys.argv) < 2:
        len_unit = 200
    else:
        try:
            len_unit = int(sys.argv[1])
        except ValueError:
            len_unit = 200

    generate_image(len_unit).save('out.png')
    print('Done!')
