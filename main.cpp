// 30.05.25 – 22.02.25

#include <SFML/Graphics.hpp>
#define WIDTH 800
#define HEIGHT 600

void draw_mandelbrot (sf::Image &image, double left, double top, double x_side,
                      double y_side, int max_iter) {
    double xscale = x_side/WIDTH, yscale = y_side/HEIGHT;
    // compute each pixel's corresponding complex number
    #pragma omp parallel for schedule(dynamic)
    for (int y=0; y<HEIGHT; y++) {
        for (int x=0; x<WIDTH; x++) {
            // constant, c
            double c_re = x*xscale + left, c_im = y*yscale + top; // re, im
            // complex, z
            double z_re = 0.0, z_im = 0.0; // re, im
            double temp_re;
            int count = 0;
            while (count<max_iter) {
                // recurrence relationship: z = z*z + c
                temp_re = z_re*z_re - z_im*z_im + c_re;
                z_im = 2.0*z_re*z_im + c_im;
                z_re = temp_re;
                if (z_re*z_re + z_im*z_im >= 2.0) break; // if diverging
                count++;
            }
            // mapping iteration count to colour
            sf::Color colour;
            if (count == max_iter) { // black if in the set
                colour = sf::Color::Black;
            } else { 
                // smooth colouring
                double log_z = log(z_re*z_re + z_im*z_im)/2.0;
                double smooth_iter = count+1 - log(log_z/log(2))/log(2);
                // normalise and raise to 0.8 for smoothing, then make purple
                double t = pow(smooth_iter/max_iter, 0.8);
                colour = sf::Color((uint8_t)(t*200), (uint8_t)(t*100), (uint8_t)(t*255));
            }
            image.setPixel(sf::Vector2u(x,y), colour);
        }
    }
}

void zoom(double z_f, double mouse_x, double mouse_y, double *x_side,
          double *y_side, double *left, double *top, int *max_iter,
          bool *modified_view) {
    // conversion between screen & fractal coordinates
    double f_x = *left + mouse_x * (*x_side / WIDTH);
    double f_y = *top + mouse_y * (*y_side / HEIGHT);
    // zoom toward mouse location
    *x_side *= z_f;
    *y_side *= z_f;
    *left = f_x - (mouse_x/WIDTH) * *x_side;
    *top = f_y - (mouse_y/HEIGHT) * *y_side;
    // adjust number of maximum iterations to add detail
    int new_iter = 10*log2(1 / *x_side);
    if ((new_iter > 30) && (new_iter < 2000)) {
        *max_iter = new_iter;
    }
    *modified_view = true;
}

// plotting the fractal
int main() {
    sf::RenderWindow window(sf::VideoMode({WIDTH,HEIGHT}), "Plotting the Mandelbrot Set with C++");
    // initial viewpoint parameters
    double left = -2.0;
    double top = -1.5;
    double x_side = 3.0;
    double y_side = 3.0 * HEIGHT/WIDTH;
    int max_iter = 30;
    // blank image
    sf::Image image({WIDTH, HEIGHT}, sf::Color::Black);
    sf::Texture texture;
    sf::Sprite sprite(texture);
    // prevent redundant calculations
    bool modified_view = true;
    // draw mandelbrot set
    draw_mandelbrot(image, left, top, x_side, y_side, max_iter);
    // panning & zoom variables
    bool is_panning = false;
    sf::Vector2i pan_start_pos;
    double pan_x = left, pan_y = top;

    // main program loop
    while (window.isOpen()) {
        while (const std::optional<sf::Event> event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) window.close();
            else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Q)) {
                if (x_side >= 3.0 || y_side >= 3.0*HEIGHT/WIDTH) continue;
                zoom(1.03, WIDTH/2.0, HEIGHT/2.0, &x_side, &y_side, &left, &top,
                     &max_iter, &modified_view);
            }
            if (const auto* scrolled = event->getIf<sf::Event::MouseWheelScrolled>()) {
                zoom((scrolled->delta > 0) ? 0.6 : 1.2,
                     (double)(scrolled->position.x),
                     (double)(scrolled->position.y),
                     &x_side, &y_side, &left, &top, &max_iter, &modified_view);
            } else if (const auto* pressed = event->getIf<sf::Event::MouseButtonPressed>()) {
                // panning start
                pan_start_pos = sf::Mouse::getPosition(window);
                pan_x = left, pan_y = top;
                is_panning = true, modified_view = false;
            } else if (event->is<sf::Event::MouseButtonReleased>()) {
                // panning end
                is_panning = false, modified_view = false;
            } else if (is_panning && event->is<sf::Event::MouseMoved>()) {
                // panning move (while pressed)
                sf::Vector2i mouse_pos = sf::Mouse::getPosition(window);
                sf::Vector2i delta = mouse_pos - pan_start_pos;
                // convert to screen coordinates (negative to pan correctly)
                double delta_x = -delta.x*(x_side/WIDTH);
                double delta_y = -delta.y*(y_side/HEIGHT);
                left = pan_x+delta_x, top = pan_y+delta_y;
                modified_view = true;
            }
        }
        if (modified_view) {
            draw_mandelbrot(image, left, top, x_side, y_side, max_iter);
            bool ret = texture.loadFromImage(image);
            if (!ret) continue;
            sprite.setTexture(texture, true);
            modified_view = false;
        }
        // clear & update
        window.clear();
        window.draw(sprite);
        window.display();
    }
    return 0;
}