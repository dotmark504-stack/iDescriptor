#include "theme.h"
#include <QFile>

namespace UiTheme {

QString toRgba(const QColor &color)
{
    return QString("rgba(%1, %2, %3, %4)")
        .arg(color.red())
        .arg(color.green())
        .arg(color.blue())
        .arg(QString::number(color.alphaF(), 'f', 3));
}

QString alphaColor(const QColor &base, int alpha)
{
    QColor copy = base;
    copy.setAlpha(alpha);
    return toRgba(copy);
}

static QString readGlobalStyleTemplate()
{
    QFile file(":/resources/styles/global.qss");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QString();
    }

    return QString::fromUtf8(file.readAll());
}

QString styleSheetForPalette(const QPalette &palette)
{
    QString stylesheet = readGlobalStyleTemplate();
    if (stylesheet.isEmpty()) {
        return stylesheet;
    }

    const QColor surface = palette.color(QPalette::Base);
    const QColor elevated = surface.lighter(palette.color(QPalette::Window).lightness() < 128 ? 112 : 103);
    const QColor textPrimary = palette.color(QPalette::WindowText);
    const QColor textSecondary = palette.color(QPalette::Text);
    const QColor accent = palette.color(QPalette::Highlight);
    const QColor accentText = palette.color(QPalette::HighlightedText);
    const QColor danger(255, 59, 48);
    const QColor success(52, 199, 89);

    const QList<QPair<QString, QString>> replacements {
        {"@CARD_RADIUS", QString::number(Tokens::CardRadius)},
        {"@CARD_RADIUS_SMALL", QString::number(Tokens::CardRadiusSmall)},
        {"@SPACING_XS", QString::number(Tokens::SpacingXs)},
        {"@SPACING_SM", QString::number(Tokens::SpacingSm)},
        {"@SPACING_MD", QString::number(Tokens::SpacingMd)},
        {"@SPACING_LG", QString::number(Tokens::SpacingLg)},
        {"@BUTTON_HEIGHT", QString::number(Tokens::ButtonHeight)},
        {"@BUTTON_HEIGHT_LARGE", QString::number(Tokens::ButtonHeightLarge)},
        {"@ICON_SMALL", QString::number(Tokens::IconSmall)},
        {"@TEXT_PRIMARY", toRgba(textPrimary)},
        {"@TEXT_SECONDARY", alphaColor(textSecondary, 180)},
        {"@SURFACE", toRgba(surface)},
        {"@SURFACE_ELEVATED", toRgba(elevated)},
        {"@BORDER_SUBTLE", alphaColor(textPrimary, 48)},
        {"@BORDER_STRONG", alphaColor(textPrimary, 85)},
        {"@ACCENT", toRgba(accent)},
        {"@ACCENT_HOVER", toRgba(accent.lighter(110))},
        {"@ACCENT_PRESSED", toRgba(accent.darker(110))},
        {"@ACCENT_TEXT", toRgba(accentText)},
        {"@SUCCESS", toRgba(success)},
        {"@SUCCESS_HOVER", toRgba(success.lighter(110))},
        {"@SUCCESS_PRESSED", toRgba(success.darker(110))},
        {"@DANGER", toRgba(danger)},
        {"@DANGER_SOFT", alphaColor(danger, 65)},
        {"@STATUS_BG", alphaColor(textPrimary, 18)},
    };

    for (const auto &entry : replacements) {
        stylesheet.replace(entry.first, entry.second);
    }

    return stylesheet;
}

void applyGlobalTheme(QApplication &app)
{
    app.setStyleSheet(styleSheetForPalette(app.palette()));
}

} // namespace UiTheme
