#include "widgets/projectswidget.h"
#include "editors/fileeditor.h"
#include "base/application.h"

ProjectsWidget::ProjectsWidget(QWidget *parent) : QWidget(parent)
{
    model = nullptr;

    welcome = new WelcomeActionViewer(this);

    stack = new QStackedLayout(this);
    stack->setMargin(0);
    stack->addWidget(welcome);

    actionSave = new QAction(app->icons.get("save.png"), QString(), this);
    actionSaveAs = new QAction(app->icons.get("save-as.png"), QString(), this);
    actionSave->setEnabled(false);
    actionSaveAs->setEnabled(false);
    actionNone = new QAction(QString(), this);
    actionNone->setEnabled(false);
    addAction(actionSave);
    addAction(actionSaveAs);
    Toolbar::addToPool("save", actionSave);
    Toolbar::addToPool("save-as", actionSaveAs);
    connect(actionSave, &QAction::triggered, this, &ProjectsWidget::saveCurrentTab);
    connect(actionSaveAs, &QAction::triggered, this, &ProjectsWidget::saveCurrentTabAs);

    connect(this, &ProjectsWidget::currentTabChanged, [=](Viewer *tab) {
        const bool isEditor = qobject_cast<Editor *>(tab);
        const bool isFileEditor = qobject_cast<FileEditor *>(tab);
        actionSave->setEnabled(isEditor);
        actionSaveAs->setEnabled(isFileEditor);
    });

    retranslate();
}

void ProjectsWidget::setModel(ProjectItemsModel *model)
{
    if (this->model) {
        disconnect(this->model, &ProjectItemsModel::added, this, &ProjectsWidget::addProject);
        disconnect(this->model, &ProjectItemsModel::removed, this, &ProjectsWidget::removeProject);
    }
    this->model = model;
    connect(model, &ProjectItemsModel::added, this, &ProjectsWidget::addProject);
    connect(model, &ProjectItemsModel::removed, this, &ProjectsWidget::removeProject);
}

void ProjectsWidget::addProject(Project *project)
{
    if (Q_LIKELY(!map.contains(project))) {
        ProjectTabsWidget *tabs = new ProjectTabsWidget(project, this);
        map.insert(project, tabs);
        stack->addWidget(tabs);
        connect(tabs, &ProjectTabsWidget::currentChanged, [=](int index) {
            Q_UNUSED(index)
            auto tab = qobject_cast<Viewer *>(tabs->currentWidget());
            emit currentTabChanged(tab);
        });
    }
}

bool ProjectsWidget::setCurrentProject(Project *project)
{
    bool result;
    ProjectTabsWidget *tabs = map.value(project, nullptr);
    if (tabs) {
        stack->setCurrentWidget(tabs);
        auto tab = qobject_cast<Viewer *>(tabs->currentWidget());
        emit currentTabChanged(tab);
        result = true;
    } else {
        stack->setCurrentWidget(welcome);
        emit currentTabChanged(welcome);
        result = false;
    }
    emit currentProjectChanged(project);
    return result;
}

void ProjectsWidget::removeProject(Project *project)
{
    delete map.value(project);
    map.remove(project);
}

bool ProjectsWidget::saveCurrentProject()
{
    return getCurrentProjectTabs()->saveProject();
}

bool ProjectsWidget::installCurrentProject()
{
    return getCurrentProjectTabs()->installProject();
}

bool ProjectsWidget::exploreCurrentProject()
{
    return getCurrentProjectTabs()->exploreProject();
}

bool ProjectsWidget::closeCurrentProject()
{
    return getCurrentProjectTabs()->closeProject();
}

bool ProjectsWidget::saveCurrentTab()
{
    auto editor = qobject_cast<Editor *>(getCurrentProjectTab());
    if (editor) {
        return editor->save();
    }
    return false;
}

bool ProjectsWidget::saveCurrentTabAs()
{
    auto fileEditor = qobject_cast<FileEditor *>(getCurrentProjectTab());
    if (fileEditor) {
        return fileEditor->saveAs();
    }
    return false;
}

ProjectActionViewer *ProjectsWidget::openProjectTab()
{
    return getCurrentProjectTabs()->openProjectTab();
}

TitleEditor *ProjectsWidget::openTitlesTab()
{
    return getCurrentProjectTabs()->openTitlesTab();
}

Viewer *ProjectsWidget::openResourceTab(const QModelIndex &index)
{
    return getCurrentProjectTabs()->openResourceTab(index);
}

bool ProjectsWidget::hasUnsavedProjects()
{
    QMapIterator<Project *, ProjectTabsWidget *> it(map);
    while (it.hasNext()) {
        it.next();
        auto tabs = it.value();
        if (tabs->isUnsaved()) {
            setCurrentProject(it.key());
            return true;
        }
    }
    return false;
}

Project *ProjectsWidget::getCurrentProject() const
{
    return map.key(getCurrentProjectTabs(), nullptr);
}

QList<QAction *> ProjectsWidget::getCurrentTabActions() const
{
    auto tab = getCurrentProjectTab();
    if (tab) {
        auto actions = tab->actions();
        if (!actions.isEmpty()) {
            return actions;
        }
    }
    return {actionNone};
}

void ProjectsWidget::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange) {
        retranslate();
    }
    QWidget::changeEvent(event);
}

ProjectTabsWidget *ProjectsWidget::getCurrentProjectTabs() const
{
    return qobject_cast<ProjectTabsWidget *>(stack->currentWidget());
}

Viewer *ProjectsWidget::getCurrentProjectTab() const
{
    auto tabs = getCurrentProjectTabs();
    return tabs ? qobject_cast<Viewer *>(tabs->currentWidget()) : nullptr;
}

void ProjectsWidget::retranslate()
{
    actionSave->setText(tr("&Save"));
    actionSaveAs->setText(tr("Save &As..."));
    actionNone->setText(tr("No actions"));
}
