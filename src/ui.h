#pragma once

#include "view.h"

#include <SFML/Graphics.hpp>

struct UICard {
    std::shared_ptr<sf::RenderTexture> rtext;

    static constexpr size_t width = 200;
    static constexpr size_t height = width * 1.4;

    UICard(const Card &card, const sf::Font &font)
    : rtext(std::make_shared<sf::RenderTexture>())
    {

        auto text = sf::Text();
        text.setFont(font);
        auto s = std::to_string(card.face_value) + " " + card.label;
        auto n = s.find('(');
        if (n != std::string::npos) {
            s.insert(n, "\n");
        }
        text.setString(s);
        text.setCharacterSize(36);
        text.setColor(sf::Color::Black);

        if (!rtext->create(width, height)) {
            ERROR("could not create texture");
        }
        rtext->clear(sf::Color::White);

        sf::Texture paper;
        paper.setSmooth(true);
        paper.loadFromFile("resources/paper.jpg");

        sf::Sprite sprite(paper);
        sprite.setScale(width / sprite.getLocalBounds().width,
                        height / sprite.getLocalBounds().height);
        rtext->draw(sprite);

        text.setPosition(4, 4);
        rtext->draw(text);
        rtext->display();
    }

    const sf::Texture &texture() const {
        assert(rtext);
        return rtext->getTexture();
    }

    static std::vector<UICard> cards;

    static void RenderCards(const sf::Font &font) {
        cards.clear();
        for (CardRef i=0; i < NUM_CARDS; ++i) {
            cards.emplace_back(UICard(Card::Get(i), font));
        }
    }
};

std::vector<UICard> UICard::cards;

// ---------------------------------------------------------------------------

struct MonkeyChallenge : public std::enable_shared_from_this<MonkeyChallenge> {
    using Ptr = std::shared_ptr<MonkeyChallenge>;

    ViewState view_state;
    State state;
    size_t round_num;
    std::thread t;

    std::mutex mtx;
    State clone;

    sf::RenderWindow window;
    sf::Font font;
    sf::Texture felt_tx;
    sf::Sprite felt_sprite;

    explicit MonkeyChallenge(size_t players)
    : state(players)
    , clone(players)
#ifdef DISPLAY_UI
    , window(sf::VideoMode(2048, 1536), "Monkey", sf::Style::Default)
#endif
    {
        TRACE();
#ifdef DISPLAY_UI
        window.setVerticalSyncEnabled(true);

        if (!font.loadFromFile("resources/ShrimpFriedRiceNo1.ttf")) {
            ERROR("could not load font");
        }
        UICard::RenderCards(font);
        felt_tx.loadFromFile("resources/felt.jpg");
        felt_sprite.setTexture(felt_tx);
        felt_sprite.setScale(2048 / felt_sprite.getLocalBounds().width,
                             1536 / felt_sprite.getLocalBounds().height);
#endif

        state.init();
    }


    void doClone() {
        std::lock_guard<std::mutex> lock(mtx);
        clone = state;
    }


    Ptr Self() {
        return MonkeyChallenge::shared_from_this();
    }

    void play() {
        TRACE();

        auto self = Self();
        t = std::thread([self] {
            auto f = [self] { self->doClone(); };
            self->state.reset();
            // TODO:
#if 0
            self->state.playGame(f);
            self->state.deck->print_size();
            self->state.deck->shuffle();
            self->state.deck->print_size();
#endif
        });
#ifdef DISPLAY_UI
        t.detach();
#else
        t.join();
#endif
    }

    void loop() {
#ifdef DISPLAY_UI
        doClone();
        while (window.isOpen()) {
            auto event = sf::Event();
            while (window.pollEvent(event)) {
                if (event.type == sf::Event::Closed) {
                    window.close();
                }
            }
            
            window.clear(sf::Color::White);
            window.draw(felt_sprite);
            
            {
                std::lock_guard<std::mutex> lock(mtx);
                view_state.update(clone);
                renderPlayers();
            }
            window.display();
        }
#endif
    }

    void renderCard(int x, int y, const ViewCard &c) {
        auto proto = Card::Get(c.card).prototype;
        sf::Sprite sprite;
        assert(proto < UICard::cards.size());
        sprite.setTexture(UICard::cards[proto].texture());
        auto tx = UICard::width >> 1;
        auto ty = UICard::height >> 1;
        sprite.setOrigin(tx, ty);
        sprite.rotate(c.rotation);
        sprite.setPosition(x+tx, y+ty);
        window.draw(sprite);
    }

    static constexpr size_t score_font_size = 60;
    static constexpr size_t score_x_offset  = 10;
    static constexpr size_t font_size       = 24;
    static constexpr size_t stack_y_offset  = font_size + 10;
    static constexpr size_t stack_x_offset  = (UICard::width >> 1) + 15;

    static constexpr size_t initial_y       = 40;
    static constexpr size_t initial_x       = score_font_size * 2 + stack_x_offset;

    static constexpr size_t char_x_offset   = stack_x_offset * 2 + 30 + UICard::width;
    static constexpr size_t char_y_offset   = stack_y_offset * 4 + UICard::height;

    void renderCharacter(int x, int y, const ViewCharacter &vc) {
        renderCard(x, y, vc.character);

        int sx = x - stack_x_offset;
        int sy = y + stack_y_offset;
        for (const auto & s: vc.styles) {
            renderCard(sx, sy, s);
            sy += stack_y_offset;
        }

        int wx = x + stack_x_offset;
        int wy = y + stack_y_offset;
        for (const auto & w : vc.weapons) {
            renderCard(wx, wy, w);
            wy += stack_y_offset;
        }
    }

    void renderScore(const Player &p, int x, int y) {
        auto text = sf::Text();
        text.setFont(font);
        auto s = std::to_string(p.visible.played_value) + "\n" + std::to_string(p.score);
        text.setString(s);
        text.setCharacterSize(score_font_size);
        text.setColor(sf::Color::Black);

        text.setPosition(x+score_x_offset, y);
        window.draw(text);
    }

    void renderPlayers() {
        int y = initial_y;
        size_t i = 0;
        for (const auto &p : state.players) {

            int x = initial_x;

            renderScore(p, 0, y);

            for (const auto &c: view_state.players[i].characters) {
                renderCharacter(x, y, c);
                x += char_x_offset;
            }
            y += char_y_offset;
            ++i;
        }
    }
};
