# include <application.h>
# define IMGUI_DEFINE_MATH_OPERATORS
# include <imgui_internal.h>

# include "crude_blueprint.h"
# include "crude_blueprint_library.h"
# include "crude_layout.h"
# include "imgui_extras.h"
# include "imgui_blueprint_utilities.h"
# include "imgui_node_editor.h"

# include "crude_json.h"

namespace ed = ax::NodeEditor;

using namespace crude_blueprint;
using namespace crude_blueprint_utilities;

static ed::EditorContext* g_Editor = nullptr;
static Blueprint g_Blueprint;

static EntryPointNode* FindEntryPointNode();

const char* Application_GetName()
{
    return "Blueprints2";
}

void Application_Initialize()
{
    using namespace crude_blueprint;

    auto printNode2Node = g_Blueprint.CreateNode<PrintNode>();
    auto entryPointNode = g_Blueprint.CreateNode<EntryPointNode>();
    auto printNode1Node = g_Blueprint.CreateNode<PrintNode>();
    auto flipFlopNode = g_Blueprint.CreateNode<FlipFlopNode>();
    auto toStringNode = g_Blueprint.CreateNode<ToStringNode>();
    auto doNNode = g_Blueprint.CreateNode<DoNNode>();
    auto addNode = g_Blueprint.CreateNode<AddNode>();

    entryPointNode->m_Exit.m_Link = &doNNode->m_Enter;

    doNNode->m_N.m_Value = 5;
    doNNode->m_Exit.m_Link = &flipFlopNode->m_Enter;

    addNode->SetType(PinType::Int32);
    addNode->m_A->m_Link = &doNNode->m_Counter;
    static_cast<Int32Pin*>(addNode->m_B.get())->m_Value = 3;

    toStringNode->m_Value.m_Link = addNode->m_Result.get();
    toStringNode->m_Exit.m_Link = &printNode2Node->m_Enter;

    printNode1Node->m_String.m_Value = "FlipFlop slot A!";
    printNode2Node->m_String.m_Value = "FlipFlop slot B!";
    printNode2Node->m_String.m_Link = &toStringNode->m_String;

    //printNode2Node->m_String.m_Link = &toStringNode.m_Enter;

    flipFlopNode->m_A.m_Link = &printNode1Node->m_Enter;
    flipFlopNode->m_B.m_Link = &toStringNode->m_Enter;


    //auto constBoolNode  = g_Blueprint.CreateNode<ConstBoolNode>();
    //auto toStringNode   = g_Blueprint.CreateNode<ToStringNode>();
    //auto printNodeNode  = g_Blueprint.CreateNode<PrintNode>();

    //crude_json::value value;
    //g_Blueprint.Save(value);
    //auto yyy = value.dump();

    //g_Blueprint.Execute(*entryPointNode);

    //Blueprint b2;

    //b2.Load(value);

    //crude_json::value value2;
    //b2.Save(value2);
    //auto zzz = value2.dump();

    //bool ok = yyy == zzz;

    //auto b3 = b2;

    g_Blueprint.CreateNode<BranchNode>();
    g_Blueprint.CreateNode<DoNNode>();
    g_Blueprint.CreateNode<DoOnceNode>();
    g_Blueprint.CreateNode<FlipFlopNode>();
    g_Blueprint.CreateNode<ForLoopNode>();
    g_Blueprint.CreateNode<GateNode>();
    g_Blueprint.CreateNode<AddNode>();


    ed::Config config;
    config.SettingsFile = "blueprint2-example.cfg";


    g_Editor = ed::CreateEditor(&config);

    //g_Blueprint.Start(*FindEntryPointNode());
}

void Application_Finalize()
{
    ed::DestroyEditor(g_Editor);
}

static EntryPointNode* FindEntryPointNode()
{
    for (auto& node : g_Blueprint.GetNodes())
    {
        if (node->GetTypeInfo().m_Id == EntryPointNode::GetStaticTypeInfo().m_Id)
        {
            return static_cast<EntryPointNode*>(node);
        }
    }

    return nullptr;
}

static const char* StepResultToString(StepResult stepResult)
{
    switch (stepResult)
    {
        case StepResult::Success:   return "Success";
        case StepResult::Done:      return "Done";
        case StepResult::Error:     return "Error";
        default:                    return "";
    }
}

static void ShowControlPanel()
{
    auto entryNode = FindEntryPointNode();

    ImEx::ScopedItemFlag disableItemFlag(entryNode == nullptr);
    ImGui::GetStyle().Alpha = entryNode == nullptr ? 0.5f : 1.0f;

    if (ImGui::Button("Start"))
    {
        g_Blueprint.Start(*entryNode);
    }

    ImGui::SameLine();
    if (ImGui::Button("Step"))
    {
        g_Blueprint.Step();
    }

    ImGui::SameLine();
    if (ImGui::Button("Run"))
    {
        g_Blueprint.Execute(*entryNode);
    }

    ImGui::SameLine();
    ImGui::Text("Status: %s", StepResultToString(g_Blueprint.LastStepResult()));

    if (auto currentNode = g_Blueprint.CurrentNode())
    {
        ImGui::SameLine();
        ImGui::Spacing();
        ImGui::SameLine();
        ImGui::Text("Current Node: %*s", static_cast<int>(currentNode->m_Name.size()), currentNode->m_Name.data());

        auto nextNode = g_Blueprint.NextNode();
        auto nextNodeName = nextNode ? nextNode->m_Name : "-";
        ImGui::SameLine();
        ImGui::Spacing();
        ImGui::SameLine();
        ImGui::Text("Next Node: %*s", static_cast<int>(nextNodeName.size()), nextNodeName.data());
    }

    ImGui::GetStyle().Alpha = 1.0f;
}

void Application_Frame()
{
    DebugOverlay debugValueRenderer(g_Blueprint);

    ShowControlPanel();

    ImGui::Separator();

    ed::SetCurrentEditor(g_Editor);

    ed::Begin("###main_editor");

    const auto iconSize = ImVec2(ImGui::GetTextLineHeight(), ImGui::GetTextLineHeight());

    debugValueRenderer.Begin();

    // Commit all nodes to editor
    for (auto& node : g_Blueprint.GetNodes())
    {
        ed::BeginNode(node->m_Id);

        //
        // General node layout:
        //
        // +-----------------------------------+
        // | Title                             |
        // | +-----------[ Dummy ]-----------+ |
        // | +---------------+   +-----------+ |
        // | | o Pin         |   |   Out B o | |
        // | | o Pin <Value> |   |   Out A o | |
        // | | o Pin         |   |           | |
        // | +---------------+   +-----------+ |
        // +-----------------------------------+

        if (!node->m_Name.empty())
        {
            ImGui::PushFont(Application_HeaderFont());
            ImGui::TextUnformatted(node->m_Name.data(), node->m_Name.data() + node->m_Name.size());
            ImGui::PopFont();
        }

        ImGui::Dummy(ImVec2(100.0f, 0.0f)); // For minimum node width

        crude_layout::Grid layout;
        layout.Begin(node->m_Id, 2, 100.0f);
        layout.SetColumnAlignment(0.0f);

        for (auto& pin : node->GetInputPins())
        {
            ImGui::Spacing(); // Add a bit of spacing to separate pins and make value not cramped
            ed::BeginPin(pin->m_Id, ed::PinKind::Input);
            ed::PinPivotAlignment(ImVec2(0.0f, 0.5f));
            ImEx::Icon(iconSize,
                PinTypeToIconType(pin->m_Type),
                g_Blueprint.IsPinLinked(pin),
                PinTypeToIconColor(pin->m_Link ? pin->m_Link->m_Type : pin->m_Type));
            if (!pin->m_Name.empty())
            {
                ImGui::SameLine();
                ImGui::TextUnformatted(pin->m_Name.data(), pin->m_Name.data() + pin->m_Name.size());
            }
            if (!g_Blueprint.IsPinLinked(pin))
            {
                ImGui::SameLine();
                EditOrDrawPinValue(*pin);
            }
            ed::EndPin();

            // Show value of the pin if node is currently executed
            debugValueRenderer.DrawInputPin(*pin);

            layout.NextRow();
        }

        layout.SetColumnAlignment(1.0f);
        layout.NextColumn();

        for (auto& pin : node->GetOutputPins())
        {
            ImGui::Spacing(); // Add a bit of spacing to separate pins and make value not cramped
            ed::BeginPin(pin->m_Id, ed::PinKind::Output);
            ed::PinPivotAlignment(ImVec2(1.0f, 0.5f));
            if (!pin->m_Name.empty())
            {
                ImGui::TextUnformatted(pin->m_Name.data(), pin->m_Name.data() + pin->m_Name.size());
                ImGui::SameLine();
            }
            ImEx::Icon(iconSize,
                PinTypeToIconType(pin->m_Type),
                g_Blueprint.IsPinLinked(pin),
                PinTypeToIconColor(pin->m_Type));
            ed::EndPin();

            // Show value of the pin if node is currently executed
            debugValueRenderer.DrawOutputPin(*pin);

            layout.NextRow();
        }

        layout.End();

        ed::EndNode();

        debugValueRenderer.DrawNode(*node);
    }

    // Commit all links to editor
    for (auto& pin : g_Blueprint.GetPins())
    {
        if (!pin->m_Link)
            continue;

        ed::Link(pin->m_Link->m_Id, pin->m_Id, pin->m_Link->m_Id, PinTypeToIconColor(pin->m_Link->m_Type));
    }

    debugValueRenderer.End();

    ed::End();


}