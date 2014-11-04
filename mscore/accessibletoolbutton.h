
namespace Ms {

#ifndef __ACCESSIBLE_TOOL_BUTTON__
#define __ACCESSIBLE_TOOL_BUTTON__
/*
 * This class inherits QToolButton and allows tabbing through the tool bar's buttons
 */
class AccessibleToolButton : public QToolButton{
      Q_OBJECT
public:
      AccessibleToolButton(QWidget* parent);
      AccessibleToolButton(QWidget* parent, QAction* defaultQAction );

      void focusInEvent(QFocusEvent* e);
      void focusOutEvent(QFocusEvent* e);
      void keyPressEvent(QKeyEvent *e);
      };

#endif
}

