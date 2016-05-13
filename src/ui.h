#pragma once

#include "view.h"
#include "game.h"

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
        std::set<CardRef> seen;
        for (CardRef i=0; i < NUM_CARDS; ++i) {
            auto card = Card::Get(i);
            if (!Contains(seen, card.prototype)) {
                cards.emplace_back(UICard(card, font));
                seen.insert(seen.begin(), card.prototype);
            }
        }
    }
};

// ---------------------------------------------------------------------------

struct GameUI : public std::enable_shared_from_this<GameUI> {
    using Ptr = std::shared_ptr<GameUI>;

    std::mutex mtx;
    ViewState  view_state;
    Game       game;
    State      clone;

    sf::RenderWindow window;
    sf::Font         font;
    sf::Texture      felt_tx;
    sf::Sprite       felt_sprite;

    explicit GameUI(uint8_t players)
    : game(players)
    , clone(players)
    , window(sf::VideoMode(2048, 1536), "Monkey", sf::Style::Default)
    {
        TRACE();

        window.setVerticalSyncEnabled(true);

        LOG("Load font");
        if (!font.loadFromFile("resources/ShrimpFriedRiceNo1.ttf")) {
            ERROR("could not load font");
        }
        LOG("Render cards");
        UICard::RenderCards(font);

        LOG("Load background");
        felt_tx.loadFromFile("resources/felt.jpg");
        felt_sprite.setTexture(felt_tx);
        felt_sprite.setScale(2048 / felt_sprite.getLocalBounds().width,
                             1536 / felt_sprite.getLocalBounds().height);
    }

    static Ptr New(uint8_t num_players=4) {
        return std::make_shared<GameUI>(num_players);
    }


    void doClone() {
        TRACE();

        std::lock_guard<std::mutex> lock(mtx);
        clone = game.state;
    }


    Ptr Self() {
        TRACE();

        return shared_from_this();
    }

    void launchGame() {
        TRACE();

        auto self = Self();
        auto thread = std::thread([self] {
            self->game.play([self] {
                self->doClone();
            });
        });
        thread.detach();
    }

    void play() {
        TRACE();

        doClone();
        launchGame();
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
    }

    void renderCard(int x, int y, const ViewCard &c) {
        TRACE();

        sf::Sprite sprite;
        assert(c.card < UICard::cards.size());
        sprite.setTexture(UICard::cards[c.card].texture());
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
        TRACE();

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
        for (const auto &p : clone.players) {

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
