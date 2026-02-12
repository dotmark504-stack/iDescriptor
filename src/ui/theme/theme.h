#ifndef IDESCRIPTOR_THEME_H
#define IDESCRIPTOR_THEME_H

#include <QApplication>
#include <QColor>
#include <QPalette>
#include <QString>

namespace UiTheme {

namespace Tokens {
inline constexpr int CardRadius = 10;
inline constexpr int CardRadiusSmall = 8;
inline constexpr int SpacingXs = 4;
inline constexpr int SpacingSm = 8;
inline constexpr int SpacingMd = 12;
inline constexpr int SpacingLg = 16;
inline constexpr int ButtonHeight = 36;
inline constexpr int ButtonHeightLarge = 40;
inline constexpr int IconSmall = 16;
inline constexpr int IconMedium = 20;
inline constexpr int IconLarge = 24;
} // namespace Tokens

QString toRgba(const QColor &color);
QString alphaColor(const QColor &base, int alpha);
QString styleSheetForPalette(const QPalette &palette);
void applyGlobalTheme(QApplication &app);

} // namespace UiTheme

#endif // IDESCRIPTOR_THEME_H
