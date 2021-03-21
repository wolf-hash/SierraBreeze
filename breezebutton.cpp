/*
 * Copyright 2014  Martin Gräßlin <mgraesslin@kde.org>
 * Copyright 2014  Hugo Pereira Da Costa <hugo.pereira@free.fr>
 * Copyright 2017  Igor Shovkun <igshov@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "breezebutton.h"

#include <KDecoration2/DecoratedClient>
#include <KColorUtils>

#include <QPainter>

namespace SierraBreeze
{

    using KDecoration2::ColorRole;
    using KDecoration2::ColorGroup;
    using KDecoration2::DecorationButtonType;


    //__________________________________________________________________
    Button::Button(DecorationButtonType type, Decoration* decoration, QObject* parent)
        : DecorationButton(type, decoration, parent)
        , m_animation( new QPropertyAnimation( this ) )
    {

        // setup animation
        m_animation->setStartValue( 0 );
        m_animation->setEndValue( 1.0 );
        m_animation->setTargetObject( this );
        m_animation->setPropertyName( "opacity" );
        m_animation->setEasingCurve( QEasingCurve::InOutQuad );

        // setup default geometry
        const int height = decoration->buttonHeight();
        setGeometry(QRect(0, 0, height, height));
        setIconSize(QSize( height, height ));

        // connections
        connect(decoration->client().data(), SIGNAL(iconChanged(QIcon)), this, SLOT(update()));
        connect(decoration->settings().data(), &KDecoration2::DecorationSettings::reconfigured, this, &Button::reconfigure);
        connect( this, &KDecoration2::DecorationButton::hoveredChanged, this, &Button::updateAnimationState );

        reconfigure();

    }

    //__________________________________________________________________
    Button::Button(QObject *parent, const QVariantList &args)
        // : DecorationButton(args.at(0).value<DecorationButtonType>(), args.at(1).value<Decoration*>(), parent)
        // , m_flag(FlagStandalone)
        // , m_animation( new QPropertyAnimation( this ) )
    // {}
        : Button(args.at(0).value<DecorationButtonType>(), args.at(1).value<Decoration*>(), parent)
        {
          m_flag = FlagStandalone;
          //! icon size must return to !valid because it was altered from the default constructor,
          //! in Standalone mode the button is not using the decoration metrics but its geometry
          m_iconSize = QSize(-1, -1);
        }

    //__________________________________________________________________
    Button *Button::create(DecorationButtonType type, KDecoration2::Decoration *decoration, QObject *parent)
    {
        if (auto d = qobject_cast<Decoration*>(decoration))
        {
            Button *b = new Button(type, d, parent);
            switch( type )
            {

                case DecorationButtonType::Close:
                b->setVisible( d->client().data()->isCloseable() );
                QObject::connect(d->client().data(), &KDecoration2::DecoratedClient::closeableChanged, b, &SierraBreeze::Button::setVisible );
                break;

                case DecorationButtonType::Maximize:
                b->setVisible( d->client().data()->isMaximizeable() );
                QObject::connect(d->client().data(), &KDecoration2::DecoratedClient::maximizeableChanged, b, &SierraBreeze::Button::setVisible );
                break;

                case DecorationButtonType::Minimize:
                b->setVisible( d->client().data()->isMinimizeable() );
                QObject::connect(d->client().data(), &KDecoration2::DecoratedClient::minimizeableChanged, b, &SierraBreeze::Button::setVisible );
                break;

                case DecorationButtonType::ContextHelp:
                b->setVisible( d->client().data()->providesContextHelp() );
                QObject::connect(d->client().data(), &KDecoration2::DecoratedClient::providesContextHelpChanged, b, &SierraBreeze::Button::setVisible );
                break;

                case DecorationButtonType::Shade:
                b->setVisible( d->client().data()->isShadeable() );
                QObject::connect(d->client().data(), &KDecoration2::DecoratedClient::shadeableChanged, b, &SierraBreeze::Button::setVisible );
                break;

                case DecorationButtonType::Menu:
                QObject::connect(d->client().data(), &KDecoration2::DecoratedClient::iconChanged, b, [b]() { b->update(); });
                break;

                default: break;

            }

            return b;
        }

        return nullptr;

    }

    //__________________________________________________________________
    void Button::paint(QPainter *painter, const QRect &repaintRegion)
    {
        Q_UNUSED(repaintRegion)

        if (!decoration()) return;

        painter->save();

        // translate from offset
        if( m_flag == FlagFirstInList ) painter->translate( m_offset );
        else painter->translate( 0, m_offset.y() );

        if( !m_iconSize.isValid() ) m_iconSize = geometry().size().toSize();

        // menu button
        if (type() == DecorationButtonType::Menu)
        {

            const QRectF iconRect( geometry().topLeft(), m_iconSize );
            const QPixmap pixmap = decoration()->client().data()->icon().pixmap( m_iconSize );
            painter->drawPixmap(iconRect.center() - QPoint(pixmap.width()/2, pixmap.height()/2)/pixmap.devicePixelRatio(), pixmap);

        } else {

            drawIcon( painter );

        }

        painter->restore();

    }

    //__________________________________________________________________
    void Button::drawIcon( QPainter *painter ) const
    {

       painter->setRenderHints( QPainter::Antialiasing );

        /*
        scale painter so that its window matches QRect( -1, -1, 20, 20 )
        this makes all further rendering and scaling simpler
        all further rendering is preformed inside QRect( 0, 0, 18, 18 )
        */
        painter->translate( geometry().topLeft() );

        const qreal width( m_iconSize.width() );
        painter->scale( width/20, width/20 );
        painter->translate( 1, 1 );

        // render background
        const QColor backgroundColor( this->backgroundColor() );
        if( backgroundColor.isValid() )
        {
            painter->setPen( Qt::NoPen );
            painter->setBrush( backgroundColor );
            painter->drawEllipse( QRectF( 0, 0, 18, 18 ) );
        }

        // render mark
        const QColor foregroundColor( this->foregroundColor() );
        if( foregroundColor.isValid() )
        {

            // setup painter
            QPen pen( foregroundColor );
            // QPen pen(QColor(250, 250, 250));
            pen.setCapStyle( Qt::RoundCap );
            pen.setJoinStyle( Qt::MiterJoin );
            pen.setWidthF( 1.1*qMax((qreal)1.0, 20/width ) );

            painter->setPen( pen );
            painter->setBrush( Qt::NoBrush );
            auto d = qobject_cast<Decoration*>( decoration() );
            auto c = d->client().data();

            const auto hover_hint_color = QColor(41, 43, 50, 200);
            QPen hint_pen(hover_hint_color);
            hint_pen.setCapStyle( Qt::RoundCap );
            hint_pen.setJoinStyle( Qt::MiterJoin );
            hint_pen.setWidthF( 1.5*qMax((qreal)1.0, 20/width ) );

            switch( type() )
            {

                case DecorationButtonType::Close:
                {
                  QColor button_color = QColor(242, 80, 86);
                  if (!c->isActive())
                    button_color = QColor(230, 230, 230);
                  painter->setBrush( button_color );
                  painter->setPen( Qt::NoPen );
                  painter->drawEllipse( QRectF( 0, 0, 18, 18 ) );
                  painter->setBrush( Qt::NoBrush );
                
                  painter->setPen(pen);
                    // painter->setPen(pen);
                    // it's a cross
                    painter->drawLine( QPointF( 5, 5 ), QPointF( 13, 13 ) );
                    painter->drawLine( 13, 5, 5, 13 );
                    break;
                  if ( isHovered() )
                  {
                    painter->setPen(pen);
                    painter->setBrush(QColor(230, 230, 230));

                    // painter->setPen(pen);
                    // it's a cross
                    painter->drawLine( QPointF( 5, 5 ), QPointF( 13, 13 ) );
                    painter->drawLine( 13, 5, 5, 13 );
                    break;
                  }
                  painter->setPen( pen );

                  break;
                }

                case DecorationButtonType::Maximize:
                {
                  if( isChecked() )
                    {
                        pen.setJoinStyle( Qt::RoundJoin );
                        painter->setPen( pen );

                        // painter->drawPolygon( QVector<QPointF>{
                        //     QPointF( 4, 9 ),
                        //     QPointF( 9, 4 ),
                        //     QPointF( 14, 9 ),
                        //     QPointF( 9, 14 )} );
                        painter->drawEllipse( QRectF( 4, 4, 10, 10 ) );

                    } else {
                        painter->drawPolyline( QVector<QPointF>{
                            QPointF( 4, 11 ),
                            QPointF( 9, 6 ),
                            QPointF( 14, 11 )});
                    }
                    break;
                }

                case DecorationButtonType::Minimize:
                {
                  painter->drawPolyline( QVector<QPointF>{
                        QPointF( 4, 7 ),
                        QPointF( 9, 12 ),
                        QPointF( 14, 7 ) });
                    break;
                }

                case DecorationButtonType::OnAllDesktops:
                {
                    painter->setPen( Qt::NoPen );
                    painter->setBrush( foregroundColor );

                    if( isChecked())
                    {

                        // outer ring
                        painter->drawEllipse( QRectF( 3, 3, 12, 12 ) );

                        // center dot
                        QColor backgroundColor( this->backgroundColor() );
                        auto d = qobject_cast<Decoration*>( decoration() );
                        if( !backgroundColor.isValid() && d ) backgroundColor = d->titleBarColor();

                        if( backgroundColor.isValid() )
                        {
                            painter->setBrush( backgroundColor );
                            painter->drawEllipse( QRectF( 8, 8, 2, 2 ) );
                        }

                    } else {

                        painter->drawPolygon( QVector<QPointF> {
                            QPointF( 6.5, 8.5 ),
                            QPointF( 12, 3 ),
                            QPointF( 15, 6 ),
                            QPointF( 9.5, 11.5 )} );

                        painter->setPen( pen );
                        painter->drawLine( QPointF( 5.5, 7.5 ), QPointF( 10.5, 12.5 ) );
                        painter->drawLine( QPointF( 12, 6 ), QPointF( 4.5, 13.5 ) );
                    }
                    break;
                }

                case DecorationButtonType::Shade:
                {

                    if (isChecked())
                    {

                        painter->drawLine( QPointF( 4, 5.5 ), QPointF( 14, 5.5 ) ); // painter->drawLine( 4, 5, 14, 5 );
                        painter->drawPolyline( QVector<QPointF> {
                            QPointF( 4, 8 ),
                            QPointF( 9, 13 ),
                            QPointF( 14, 8 )} );

                    } else {

                        painter->drawLine( QPointF( 4, 5.5 ), QPointF( 14, 5.5 ) ); // painter->drawLine( 4, 5, 14, 5 );
                        painter->drawPolyline(  QVector<QPointF> {
                            QPointF( 4, 13 ),
                            QPointF( 9, 8 ),
                            QPointF( 14, 13 ) });
                    }

                    break;

                }

                case DecorationButtonType::KeepBelow:
                {

                    painter->drawPolyline(  QVector<QPointF> {
                        QPointF( 4, 5 ),
                        QPointF( 9, 10 ),
                        QPointF( 14, 5 ) });

                    painter->drawPolyline(  QVector<QPointF> {
                        QPointF( 4, 9 ),
                        QPointF( 9, 14 ),
                        QPointF( 14, 9 ) });
                    break;

                }

                case DecorationButtonType::KeepAbove:
                {
                    painter->drawPolyline(  QVector<QPointF> {
                        QPointF( 4, 9 ),
                        QPointF( 9, 4 ),
                        QPointF( 14, 9 ) });

                    painter->drawPolyline(  QVector<QPointF> {
                        QPointF( 4, 13 ),
                        QPointF( 9, 8 ),
                        QPointF( 14, 13 ) });
                    break;
                }


                case DecorationButtonType::ApplicationMenu:
                {
                    painter->drawRect( QRectF( 3.5, 4.5, 11, 1 ) ); // painter->drawLine( QPointF( 3.5, 5 ), QPointF( 14.5, 5 ) );
                    painter->drawRect( QRectF( 3.5, 8.5, 11, 1 ) ); // painter->drawLine( QPointF( 3.5, 9 ), QPointF( 14.5, 9 ) );
                    painter->drawRect( QRectF( 3.5, 12.5, 11, 1 ) ); // painter->drawLine( QPointF( 3.5, 13 ), QPointF( 14.5, 13 ) );
                    break;
                }

                case DecorationButtonType::ContextHelp:
                {
                    QPainterPath path;
                    path.moveTo( 5, 6 );
                    path.arcTo( QRectF( 5, 3.5, 8, 5 ), 180, -180 );
                    path.cubicTo( QPointF(12.5, 9.5), QPointF( 9, 7.5 ), QPointF( 9, 11.5 ) );
                    painter->drawPath( path );

                    painter->drawRect( QRectF( 9, 15, 0.5, 0.5 ) ); // painter->drawPoint( 9, 15 );

                    break;
                }


                default: break;

            }

        }

    }

    //__________________________________________________________________
  QColor Button::foregroundColor() const
    {
        auto d = qobject_cast<Decoration*>( decoration() );
        QColor titleBarColor ( d->titleBarColor() );

        if( !d ) {

            return QColor();

        } else if( isPressed() ) {

            return titleBarColor;

        } else if( ( type() == DecorationButtonType::KeepBelow || type() == DecorationButtonType::KeepAbove || type() == DecorationButtonType::Shade ) && isChecked() ) {

            return titleBarColor;

        } else if( m_animation->state() == QAbstractAnimation::Running ) {
            if(type() == DecorationButtonType::Close){
                return KColorUtils::mix(titleBarColor, QColor(250, 250, 250), m_opacity);
            }

            return KColorUtils::mix(  QColor(250, 250,250), titleBarColor, m_opacity );

        } else if( isHovered() ) {

            if((type() == DecorationButtonType::Close) && (d->client().data()->isActive())){
                return QColor(250,250,250);
            }
            return titleBarColor;

        } else {

            if(type() == DecorationButtonType::Close){
                return titleBarColor;
            }
            return QColor(250, 250, 250);

        }

    }

    //__________________________________________________________________
    QColor Button::backgroundColor() const
    {
        auto d = qobject_cast<Decoration*>( decoration() );
        if( !d ) {

            return QColor();

        }

        auto c = d->client().toStrongRef().data();
        if( isPressed() ) {

            if( type() == DecorationButtonType::Close ) return c->color( ColorGroup::Warning, ColorRole::Foreground );
            else return KColorUtils::mix(  QColor(250, 250,250), d->titleBarColor(), 0.3 );

        } else if( ( type() == DecorationButtonType::KeepBelow || type() == DecorationButtonType::KeepAbove || type() == DecorationButtonType::Shade ) && isChecked() ) {

            return QColor(250, 250, 250);

        } else if( m_animation->state() == QAbstractAnimation::Running ) {

            if( type() == DecorationButtonType::Close )
            {
                QColor color( c->color( ColorGroup::Warning, ColorRole::Foreground ).lighter() );
                color.setAlpha( color.alpha()*m_opacity );
                return color;

            } else {

                QColor color( QColor(250,250,250) );
                color.setAlpha( color.alpha()*m_opacity );
                return color;

            }

        } else if( isHovered() ) {

            if( type() == DecorationButtonType::Close ) return c->color( ColorGroup::Warning, ColorRole::Foreground ).lighter();
            else return QColor(250, 250,250);

        } else {

            return QColor();

        }

    }

    //________________________________________________________________
    void Button::reconfigure()
    {

        // animation
        auto d = qobject_cast<Decoration*>(decoration());
        if( d )  m_animation->setDuration( d->internalSettings()->animationsDuration() );

    }

    //__________________________________________________________________
    void Button::updateAnimationState( bool hovered )
    {

        auto d = qobject_cast<Decoration*>(decoration());
        if( !(d && d->internalSettings()->animationsEnabled() ) ) return;

        m_animation->setDirection( hovered ? QPropertyAnimation::Forward : QPropertyAnimation::Backward );
        if( m_animation->state() != QPropertyAnimation::Running ) m_animation->start();

    }

} // namespace
