#pragma once

#include "Base/BaseTypes.h"

#include "Scene/System/EditorLODSystem.h"
#include "Scene/System/EditorStatisticsSystem.h"
#include "Tools/QtPosSaver/QtPosSaver.h"

#include <QWidget>

namespace Ui
{
class LODEditor;
}

class SceneEditor2;
class SelectableGroup;
class Command2;
class QFrame;
class QPushButton;
class LODDistanceWidget;

class LazyUpdater;
class LODEditor : public QWidget, private EditorLODSystemUIDelegate, EditorStatisticsSystemUIDelegate
{
    Q_OBJECT

public:
    explicit LODEditor(QWidget* parent = nullptr);
    ~LODEditor() override;

private slots:

    //force signals
    void ForceDistanceStateChanged(bool checked);
    void ForceDistanceChanged(int distance);
    void ForceLayerActivated(int index);

    //scene signals
    void SceneActivated(SceneEditor2* scene);
    void SceneDeactivated(SceneEditor2* scene);
    void SceneSelectionChanged(SceneEditor2* scene, const SelectableGroup* selected, const SelectableGroup* deselected);

    //distance signals
    void LODDistanceChangedByDistanceWidget();
    void LODDistanceIsChangingBySlider();
    void LODDistanceChangedBySlider();

    //mode signals
    void SceneModeToggled(bool toggled);
    void SelectionModeToggled(bool toggled);
    void RecursiveModeSelected(bool recursive);

    //action
    void CopyLastLODToLOD0Clicked();
    void CreatePlaneLODClicked();
    void DeleteLOD();

private:
    void SetupSceneSignals();
    void SetupInternalUI();

    void SetupForceUI();
    void UpdateForceSliderRange();

    void UpdatePanelsUI(SceneEditor2* forScene);
    void UpdatePanelsForCurrentScene();

    void SetupDistancesUI();
    void UpdateDistanceSpinboxesUI(const DAVA::Vector<DAVA::float32>& distances, const DAVA::Vector<bool>& multiple, DAVA::int32 count);

    void SetupActionsUI();

    //EditorLODSystemV2UIDelegate
    void UpdateModeUI(EditorLODSystem* forSystem, const eEditorMode mode, bool recursive) override;
    void UpdateForceUI(EditorLODSystem* forSystem, const ForceValues& forceValues) override;
    void UpdateDistanceUI(EditorLODSystem* forSystem, const LODComponentHolder* lodData) override;
    void UpdateActionUI(EditorLODSystem* forSystem) override;
    //end of EditorLODSystemV2UIDelegate

    //EditorStatisticsSystemUIDelegate
    void UpdateTrianglesUI(EditorStatisticsSystem* forSystem) override;
    //end of EditorStatisticsSystemUIDelegate

    EditorLODSystem* GetCurrentEditorLODSystem() const;
    EditorStatisticsSystem* GetCurrentEditorStatisticsSystem() const;

    std::unique_ptr<Ui::LODEditor> ui;

    DAVA::Vector<LODDistanceWidget*> distanceWidgets;

    LazyUpdater* panelsUpdater = nullptr;
};
