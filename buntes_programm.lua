local function color(r, g, b)
    return r * 65536 + g * 256 + b
end

local function rgb(color)
    return color >> 16, (color >> 8) & 255, color & 255
end

local function black()
    local a = { }
    for i = 1, 160 do
        a[i] = 0
    end
    return a
end

local function row_to(pixels, clr, dval)
    for i = 160, 1, -1 do
        pixels[i] = clr
        set_leds(pixels)
        delay(dval)
    end
end

local function transition(op, np, total_time)
    local in_delay = total_time // 256
    local diff = { }
    for i = 1, 160 do
        local ro, go, bo = rgb(op[i])
        local rn, gn, bn = rgb(np[i])
        diff[i] = { rn - ro, gn - go, bn - bo }
    end
    local pixels = { }
    for i = 0, 255 do
        for j = 1, 160 do
            local rc, gc, bc = rgb(op[j])
            local d = diff[j]
            pixels[j] = color(rc + (d[1] * i) // 255, gc + (d[2] * i) // 255, bc + (d[3] * i) // 255)
        end
        set_leds(pixels)
        delay(in_delay)
    end
end

for i = 1, 160 do
    local pixels = black()
    pixels[i] = color(255, 0, 0)
    set_leds(pixels)
    delay(25)
end
local pixels = black()
row_to(pixels, color(255, 0, 0), 25)
row_to(pixels, color(255, 128, 0), 25)
row_to(pixels, color(0, 255, 0), 25)
row_to(pixels, color(0, 0, 255), 25)
row_to(pixels, color(0, 0, 0), 25)

for i = 1, 80 do
    local pixels = black()
    pixels[i] = color(255, 0, 0)
    pixels[160 - i] = color(0, 0, 255)
    set_leds(pixels)
    delay(41 - i // 2)
end

for i = 1, 80 do
    pixels[81 - i] = color(100, 0, 255)
    pixels[80 + i] = color(100, 0, 255)
    set_leds(pixels)
    delay(1)
end

local oldpixels = black()
for i = 1, 2550 do
    for j = 1, 160 do
        oldpixels[j] = pixels[j]
    end
    for j = 1, 32 do
        pixels[((j + i) % 160) + 1] = color(255, 0, 0)
        pixels[((j + i + 32) % 160) + 1] = color(255, 128, 0)
        pixels[((j + i + 64) % 160) + 1] = color(100, 0, 255)
        pixels[((j + i + 96) % 160) + 1] = color(0, 0, 255)
        pixels[((j + i + 128) % 160) + 1] = color(0, 255, 0)
    end
    transition(oldpixels, pixels, 2500)
end
