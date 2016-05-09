#pragma once

#include "view.h"

#include <SFML/Graphics.hpp>

struct UICard {
    shared_ptr<sf::RenderTexture> rtext;

    static constexpr size_t width = 200;
    static constexpr size_t height = width * 1.4;

    UICard(const CardTableEntry &card, const sf::Font &font)
    : rtext(make_shared<sf::RenderTexture>())
    {

        auto text = sf::Text();
        text.setFont(font);
        auto s = to_string(card.value) + " " + card.label;
        auto n = s.find('(');
        if (n != string::npos) {
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
        paper.loadFromFile("paper.jpg");

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

    static vector<UICard> cards;

    static void RenderCards(const sf::Font &font) {
        cards.clear();
        for (size_t i=0; i < CARD_TABLE_PROTO.size(); ++i) {
            cards.emplace_back(UICard(CARD_TABLE_PROTO[i], font));
        }
    }
};

vector<UICard> UICard::cards;

// ---------------------------------------------------------------------------

struct RenderContext {
    sf::RenderWindow window;
    sf::Font font;
    sf::Texture felt_tx;
    sf::Sprite felt_sprite;

    explicit RenderContext(vector<Player::Ptr> players)
    : deck(make_shared<Deck>())
    , state(make_shared<GameState>(deck, players))
#ifdef DISPLAY_UI
    , window(sf::VideoMode(2048, 1536), "Monkey", sf::Style::Default)
#endif
    {
#ifdef DISPLAY_UI
        window.setVerticalSyncEnabled(true);

        if (!font.loadFromFile("ShrimpFriedRiceNo1.ttf")) {
            ERROR("could not load font");
        }
        UICard::RenderCards(font);
        felt_tx.loadFromFile("felt.jpg");
        felt_sprite.setTexture(felt_tx);
        felt_sprite.setScale(2048 / felt_sprite.getLocalBounds().width,
                             1536 / felt_sprite.getLocalBounds().height);
#endif

        state->init();
        deck->initialize();
    }

    mutex mtx;
    ChallengeFlags flags;
    GameState::Ptr clone;

    void doClone() {
        lock_guard<mutex> lock(mtx);
        clone = state->clone<T>();
    }

#ifdef DISPLAY_UI
        t.detach();
#else
        t.join();
#endif
    }

    ViewState view_state;

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
                lock_guard<mutex> lock(mtx);
                view_state.update(clone);
                renderPlayers();
            }
            window.display();
        }
#endif
    }

    void renderCard(int x, int y, const ViewCard &card) {
        auto proto = CARD_TABLE[card.id].prototype;
        sf::Sprite sprite;
        assert(proto < UICard::cards.size());
        sprite.setTexture(UICard::cards[proto].texture());
        auto tx = UICard::width >> 1;
        auto ty = UICard::height >> 1;
        sprite.setOrigin(tx, ty);
        sprite.rotate(card.rotation);
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

    void renderScore(Player *player, int x, int y) {
        auto text = sf::Text();
        text.setFont(font);
        auto s = to_string(player->visible.value(clone->flags)) + "\n" + to_string(player->score);
        text.setString(s);
        text.setCharacterSize(score_font_size);
        text.setColor(sf::Color::Black);

        text.setPosition(x+score_x_offset, y);
        window.draw(text);
    }

    void renderPlayers() {
        int y = initial_y;
        size_t i = 0;
        for (const auto &p : state->players) {

            int x = initial_x;

            renderScore(p.get(), 0, y);

            for (const auto &c: view_state.players[i].characters) {
                renderCharacter(x, y, c);
                x += char_x_offset;
            }
            y += char_y_offset;
            ++i;
        }
    }
};
