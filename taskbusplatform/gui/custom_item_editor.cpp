#include <QItemEditorFactory>
#include <QItemEditorCreator>
#include <QSpinBox>
#include <QLineEdit>
#include <QCheckBox>
#include <QDateEdit>
#include <QPixmap>
#include <QLabel>
#include <QTimeEdit>
#include <QDateTimeEdit>
void create_custom_item_editor()
{
	//使用自定义的编辑器 using user-defined item editor
	QItemEditorFactory *factory = new QItemEditorFactory(*QItemEditorFactory::defaultFactory());

	QItemEditorCreatorBase * creator_double = new QStandardItemEditorCreator<QLineEdit>();
	factory->registerEditor(QVariant::Double, creator_double);

	QItemEditorCreatorBase * creator_int = new QStandardItemEditorCreator<QSpinBox>();
	factory->registerEditor(QVariant::Int, creator_int);
	QItemEditorCreatorBase * creator_uint = new QStandardItemEditorCreator<QSpinBox>();
	factory->registerEditor(QVariant::UInt, creator_uint);
	QItemEditorCreatorBase * creator_Longlone = new QStandardItemEditorCreator<QLineEdit>();
	factory->registerEditor(QVariant::LongLong, creator_Longlone);
	QItemEditorCreatorBase * creator_ULonglone = new QStandardItemEditorCreator<QLineEdit>();
	factory->registerEditor(QVariant::ULongLong, creator_ULonglone);

	QItemEditorCreatorBase * creator_bool = new QStandardItemEditorCreator<QCheckBox>();
	factory->registerEditor(QVariant::Bool, creator_bool);

	QItemEditorCreatorBase * creator_string = new QStandardItemEditorCreator<QLineEdit>();
	factory->registerEditor(QVariant::String, creator_string);

	QItemEditorCreatorBase * creator_QDate = new QStandardItemEditorCreator<QDateEdit>();
	factory->registerEditor(QVariant::Date, creator_QDate);

	QItemEditorCreatorBase * creator_QDateTimeEdit = new QStandardItemEditorCreator<QDateTimeEdit>();
	factory->registerEditor(QVariant::DateTime, creator_QDateTimeEdit);

	QItemEditorCreatorBase * creator_Time = new QStandardItemEditorCreator<QTimeEdit>();
	factory->registerEditor(QVariant::Time, creator_Time);

	QItemEditorCreatorBase * creator_QPixmap = new QStandardItemEditorCreator<QLabel>();
	factory->registerEditor(QVariant::Pixmap, creator_QPixmap);

	QItemEditorFactory::setDefaultFactory(factory);
}
