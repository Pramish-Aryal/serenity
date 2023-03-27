#include <AK/Vector.h>
#include <LibCore/ArgsParser.h>
#include <LibCore/System.h>
#include <LibDesktop/Screensaver.h>
#include <LibGUI/Application.h>
#include <LibGUI/Event.h>
#include <LibGUI/Icon.h>
#include <LibGUI/Painter.h>
#include <LibGUI/Window.h>
#include <LibGfx/Bitmap.h>
#include <LibMain/Main.h>
#include <stdio.h>
#include <time.h>

struct Ball {
    int x;
    int y;
    
    const int radius;
    float vx, vy;

    operator Gfx::IntPoint() const
    {
        return { x, y };
    }
};

class Metaball final : public Desktop::Screensaver {
    C_OBJECT(Metaball)
public:
    virtual ~Metaball() override = default;
    ErrorOr<void> create_balls(int, int, int);

    void set_speed(unsigned speed) { m_speed = speed; }

private:
    Metaball(int);
    RefPtr<Gfx::Bitmap> m_bitmap;

    void draw();
    virtual void paint_event(GUI::PaintEvent&) override;
    virtual void timer_event(Core::TimerEvent&) override;
    virtual void keydown_event(GUI::KeyEvent&) override;
    virtual void mousewheel_event(GUI::MouseEvent&) override;

    Vector<Ball> m_balls;
    unsigned m_speed = 1;
    Array<float, 1000 * 1000 * 3> m_field_buffer;
    unsigned cell_size = 3;
    int rows;
    int cols;
    int m_width, m_height;
};

Metaball::Metaball(int interval)
{
    on_screensaver_exit = []() { GUI::Application::the()->quit(); };
    srand(time(nullptr));
    stop_timer();
    start_timer(interval);
}

ErrorOr<void> Metaball::create_balls(int width, int height, int balls)
{
    m_bitmap = TRY(Gfx::Bitmap::create(Gfx::BitmapFormat::BGRx8888, { width, height }));
    rows = 1 + height / cell_size;
    cols = 1 + width / cell_size;
    m_width = width;
    m_height = height;
    m_balls.grow_capacity(balls);
    for (int i = 0; i < balls; i++) {
        m_balls.append({ width / 2 + rand() % (width / 2),
            height / 2 + rand() % (height / 2),
            rand() % 25 + 5,
            (float)rand() / (float)RAND_MAX - 1.0f, (float)rand() /(float)RAND_MAX - 1.0f});
    }
    // m_field_buffer.grow_capacity(rows * cols);
    // for (int i = 0; i < rows * cols; i++) {
    //     m_field_buffer.append(0.0f);
    // }
    draw();
    return {};
}

void Metaball::keydown_event(GUI::KeyEvent& event)
{
    switch (event.key()) {
    case Key_Plus:
        m_speed++;
        break;
    case Key_Minus:
        if (--m_speed < 1)
            m_speed = 1;
        break;
    default:
        Desktop::Screensaver::keydown_event(event);
    }
}

void Metaball::mousewheel_event(GUI::MouseEvent& event)
{
    if (event.wheel_delta_y() == 0)
        return;

    m_speed += event.wheel_delta_y() > 0 ? -1 : 1;
    if (m_speed < 1)
        m_speed = 1;
}

void Metaball::paint_event(GUI::PaintEvent& event)
{
    GUI::Painter painter(*this);
    painter.add_clip_rect(event.rect());
    painter.draw_scaled_bitmap(rect(), *m_bitmap, m_bitmap->rect());
}

float lerp_pixel(int, float, int, float);
float lerp_pixel(int a, float a_val, int b, float b_val) {
	return (a + (b - a) * (1 - a_val) / (b_val - a_val));
}

void Metaball::timer_event(Core::TimerEvent&)
{
    m_bitmap->fill(Color::Black);

    GUI::Painter painter(*m_bitmap);

#define CELL(x, y)  m_field_buffer[y * cols + x]

    for(int i = 0; i < rows; ++i) {
		for(int j = 0; j < cols; ++j) {
			float s = 0;
			int x = j * cell_size;
			int y = i * cell_size;
			for(unsigned n = 0; n <  m_balls.size(); ++n) {
				s += (float)m_balls[n].radius / ( sqrtf( (x - m_balls[n].x) * (x - m_balls[n].x) + (y - m_balls[n].y) * (y - m_balls[n].y)));
			}
            CELL(j, i) = s;
		}
	}
    
#define THRESHOLD 1.0f
    for(int i = 0; i < rows; ++i) {
		for(int j = 0; j < cols; ++j) {
			int x = j * cell_size;
			int y = i * cell_size;
			//DrawCircle(x, y, 2, WHITE);
			
			int a = CELL(j, i) > THRESHOLD; 
			int b = CELL(j + 1, i) > THRESHOLD; 
			int c = CELL(j + 1, i + 1) > THRESHOLD; 
			int d = CELL(j, i + 1) > THRESHOLD; 
					
			auto p1 = Gfx::FloatPoint(float(x), float(y));
			auto p2 = Gfx::FloatPoint(float(x) + cell_size, float(y));
			auto p3 = Gfx::FloatPoint(float(x) + cell_size, float(y) + cell_size);
			auto p4 = Gfx::FloatPoint(float(x), float(y) + cell_size);
			Color color = Color{(unsigned char)(255 * x / m_width), (unsigned char)(255 * y / m_height), 0, 255};
			switch( d << 3 | c << 2 | b << 1 | a) {
				case 0: {
					//DrawCircleLines(x, y, cell_size, RED);
				} break;
			    case 1: {
					auto l1 = Gfx::FloatPoint(p1.x(), lerp_pixel(p1.y(), CELL(j, i), p4.y(), CELL(j, i + 1)));
					auto l2 = Gfx::FloatPoint(lerp_pixel(p1.x(), CELL(j, i), p2.x(), CELL(j + 1, i)), p1.y());
					//painter.draw_line(l1, l2, color);
                    painter.draw_line(Gfx::IntPoint(l1), Gfx::IntPoint(l2), color);
				} break;
					case 2: { 
						auto l1 = Gfx::FloatPoint(lerp_pixel(p1.x(), CELL(j, i), p2.x(), CELL(j + 1, i)), p2.y());
						auto l2 = Gfx::FloatPoint(p2.x(), lerp_pixel(p2.y(), CELL(j + 1, i), p3.y(), CELL(j+1, i+1)));
						painter.draw_line(Gfx::IntPoint(l1), Gfx::IntPoint(l2), color);
					} break;
					case 3: { 
						auto l1 = Gfx::FloatPoint(p1.x(), lerp_pixel(p1.y(), CELL(j, i), p4.y(), CELL(j, i + 1)));
						auto l2 = Gfx::FloatPoint(p2.x(), lerp_pixel(p2.y(), CELL(j + 1, i), p3.y(), CELL(j + 1, i + 1)));
						painter.draw_line(Gfx::IntPoint(l1), Gfx::IntPoint(l2), color);
					} break;
					case 4: { 
						auto l1 = Gfx::FloatPoint(lerp_pixel(p4.x(), CELL(j, i + 1), p3.x(), CELL(j + 1, i + 1)), p4.y());
						auto l2 = Gfx::FloatPoint(p2.x(), lerp_pixel(p2.y(), CELL(j + 1, i), p3.y(), CELL(j + 1, i + 1)));
						
						painter.draw_line(Gfx::IntPoint(l1), Gfx::IntPoint(l2), color);
					} break;
					case 5: { 
						auto l1 = Gfx::FloatPoint(lerp_pixel(p1.x(), CELL(j, i), p2.x(), CELL(j + 1, i)), p2.y());
						auto l2 = Gfx::FloatPoint(p2.x(), lerp_pixel(p2.y(), CELL(j + 1, i), p3.y(), CELL(j+1, i+1)));
						
						auto l3 = Gfx::FloatPoint(p4.x(), lerp_pixel(p1.y(), CELL(j, i), p4.y(), CELL(j, i + 1)));
						auto l4 = Gfx::FloatPoint(lerp_pixel(p4.x(), CELL(j, i + 1), p3.x(), CELL(j + 1, i + 1)), p4.y());
						
						painter.draw_line(Gfx::IntPoint(l1), Gfx::IntPoint(l2), color);
						painter.draw_line(Gfx::IntPoint(l3), Gfx::IntPoint(l4), color);
					} break;
					case 6: { 
						auto l1 = Gfx::FloatPoint(lerp_pixel(p1.x(), CELL(j, i), p2.x(), CELL(j + 1, i)), p2.y());
						auto l2 = Gfx::FloatPoint(lerp_pixel(p4.x(), CELL(j, i + 1), p3.x(), CELL(j + 1, i + 1)), p3.y());
						painter.draw_line(Gfx::IntPoint(l1), Gfx::IntPoint(l2), color);
					} break;
					case 7: { 
						auto l1 = Gfx::FloatPoint(p4.x(), lerp_pixel(p1.y(), CELL(j, i), p4.y(), CELL(j, i + 1)));
						auto l2 = Gfx::FloatPoint(lerp_pixel(p4.x(), CELL(j, i + 1), p3.x(), CELL(j + 1, i + 1)), p4.y());
						
						painter.draw_line(Gfx::IntPoint(l1), Gfx::IntPoint(l2), color);
					} break;
					case 8: { 
						auto l1 = Gfx::FloatPoint(p4.x(), lerp_pixel(p1.y(), CELL(j, i), p4.y(), CELL(j, i + 1)));
						auto l2 = Gfx::FloatPoint(lerp_pixel(p4.x(), CELL(j, i + 1), p3.x(), CELL(j + 1, i + 1)), p4.y());
						
						painter.draw_line(Gfx::IntPoint(l1), Gfx::IntPoint(l2), color);
					} break;
					case 9: { 
						auto l1 = Gfx::FloatPoint(lerp_pixel(p1.x(), CELL(j, i), p2.x(), CELL(j + 1, i)), p1.y());
						auto l2 = Gfx::FloatPoint(lerp_pixel(p4.x(), CELL(j, i + 1), p3.x(), CELL(j + 1, i + 1)), p4.y());
						painter.draw_line(Gfx::IntPoint(l1), Gfx::IntPoint(l2), color);
					} break;
					case 10: { 
						auto l1 = Gfx::FloatPoint(p1.x(), lerp_pixel(p1.y(), CELL(j, i), p4.y(), CELL(j, i + 1)));
						auto l2 = Gfx::FloatPoint(lerp_pixel(p1.x(), CELL(j, i), p2.x(), CELL(j + 1, i)), p1.y());
						auto l3 = Gfx::FloatPoint(lerp_pixel(p4.x(), CELL(j, i + 1), p3.x(), CELL(j + 1, i + 1)), p4.y());
						auto l4 = Gfx::FloatPoint(p2.x(), lerp_pixel(p2.y(), CELL(j + 1, i), p3.y(), CELL(j + 1, i + 1)));
						
						painter.draw_line(Gfx::IntPoint(l1), Gfx::IntPoint(l2), color);
						painter.draw_line(Gfx::IntPoint(l3), Gfx::IntPoint(l4), color);
					} break;
					case 11: { 
						auto l1 = Gfx::FloatPoint(lerp_pixel(p4.x(), CELL(j, i + 1), p3.x(), CELL(j + 1, i + 1)), p4.y());
						auto l2 = Gfx::FloatPoint(p2.x(), lerp_pixel(p2.y(), CELL(j + 1, i), p3.y(), CELL(j + 1, i + 1)));
						
						painter.draw_line(Gfx::IntPoint(l1), Gfx::IntPoint(l2), color);
					} break;
					case 12: { 
						auto l1 = Gfx::FloatPoint(p4.x(), lerp_pixel(p1.y(), CELL(j, i), p4.y(), CELL(j, i + 1)));
						auto l2 = Gfx::FloatPoint(p3.x(), lerp_pixel(p2.y(), CELL(j + 1, i), p3.y(), CELL(j + 1, i + 1)));
						painter.draw_line(Gfx::IntPoint(l1), Gfx::IntPoint(l2), color);
					} break;
					case 13: { 
						auto l1 = Gfx::FloatPoint(lerp_pixel(p1.x(), CELL(j, i), p2.x(), CELL(j + 1, i)), p2.y());
						auto l2 = Gfx::FloatPoint(p2.x(), lerp_pixel(p2.y(), CELL(j + 1, i), p3.y(), CELL(j+1, i+1)));
						painter.draw_line(Gfx::IntPoint(l1), Gfx::IntPoint(l2), color);
					} break;
					case 14: { 
						auto l1 = Gfx::FloatPoint(p1.x(), lerp_pixel(p1.y(), CELL(j, i), p4.y(), CELL(j, i + 1)));
						auto l2 = Gfx::FloatPoint(lerp_pixel(p1.x(), CELL(j, i), p2.x(), CELL(j + 1, i)), p1.y());
						painter.draw_line(Gfx::IntPoint(l1), Gfx::IntPoint(l2), color);
					} break;
					
					case 15: {
						//if(draw_pixels) DrawPixel(x, y, color);
					}break;
	    	}
		}
	}
    
    for(size_t i = 0; i < m_balls.size(); ++i) {
		m_balls[i].x += 5 * m_balls[i].vx;
		m_balls[i].y += 5 * m_balls[i].vy;
		
		if(m_balls[i].x + m_balls[i].radius > m_width - 50 + m_balls[i].radius) {
			m_balls[i].vx *= -1;
			m_balls[i].x = m_width - m_balls[i].radius - 50 + m_balls[i].radius;
		}
		if(m_balls[i].x - m_balls[i].radius < 50 + m_balls[i].radius) {
			m_balls[i].vx *= -1;
			m_balls[i].x = m_balls[i].radius + 50 + m_balls[i].radius;
		}
		
		if(m_balls[i].y + m_balls[i].radius > m_height - 50 + m_balls[i].radius) {
			m_balls[i].vy *= -1;
			m_balls[i].y = m_height - m_balls[i].radius - 50 + m_balls[i].radius;
		}
		if(m_balls[i].y - m_balls[i].radius < 50 + m_balls[i].radius) {
			m_balls[i].vy *= -1;
			m_balls[i].y = m_balls[i].radius + 50 + m_balls[i].radius;
		}
		
	}

    update();
}

void Metaball::draw()
{
    GUI::Painter painter(*m_bitmap);
    painter.fill_rect(m_bitmap->rect(), Color::Black);
}

ErrorOr<int> serenity_main(Main::Arguments arguments)
{
    TRY(Core::System::pledge("stdio recvfd sendfd rpath unix"));

    unsigned ball_count = 6;
    unsigned refresh_rate = 16;
    unsigned speed = 1;

    Core::ArgsParser args_parser;
    args_parser.set_general_help("The classic metaball screensaver.");
    args_parser.add_option(ball_count, "Number of balls to draw (default = 6)", "balls", 'c', "number");
    args_parser.add_option(refresh_rate, "Refresh rate (default = 16)", "rate", 'r', "milliseconds");
    args_parser.add_option(speed, "Speed (default = 1)", "speed", 's', "number");
    args_parser.parse(arguments);

    auto app = TRY(GUI::Application::try_create(arguments));

    TRY(Core::System::pledge("stdio recvfd sendfd rpath"));

    auto window = TRY(Desktop::Screensaver::create_window("Metaball"sv, "app-Metaball"sv));

    auto metaball_window = TRY(window->set_main_widget<Metaball>(refresh_rate));
    metaball_window->set_fill_with_background_color(false);
    metaball_window->set_override_cursor(Gfx::StandardCursor::Hidden);
    metaball_window->update();
    window->show();

    TRY(metaball_window->create_balls(window->width(), window->height(), ball_count));
    metaball_window->set_speed(speed);
    metaball_window->update();

    window->move_to_front();
    window->set_cursor(Gfx::StandardCursor::Hidden);
    window->update();

    return app->exec();
}
