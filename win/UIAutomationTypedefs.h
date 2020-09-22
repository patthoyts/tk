// declare missing typedefs for C import of UIA interfaces
//
// This file should be included _before_ the UIAutomation headers
// when using the UIAutomationCode headers in C programs.

#pragma once

typedef enum ConditionType ConditionType;
typedef enum PropertyConditionFlags PropertyConditionFlags;
typedef enum AutomationElementMode AutomationElementMode;
typedef enum TreeScope TreeScope;
typedef enum NavigateDirection NavigateDirection;
typedef enum NormalizeState NormalizeState;
typedef enum TreeTraversalOptions TreeTraversalOptions;
typedef enum ProviderType ProviderType;
typedef enum AutomationIdentifierType AutomationIdentifierType;
typedef enum EventArgsType EventArgsType;
typedef enum AsyncContentLoadedState AsyncContentLoadedState;
typedef enum StructureChangeType StructureChangeType;
typedef enum TextEditChangeType TextEditChangeType;
typedef enum NotificationKind NotificationKind;
typedef enum NotificationProcessing NotificationProcessing;
typedef enum DockPosition DockPosition;
typedef enum ScrollAmount ScrollAmount;
typedef enum WindowVisualState WindowVisualState;
typedef enum SupportedTextSelection SupportedTextSelection;
typedef enum TextPatternRangeEndpoint TextPatternRangeEndpoint;
typedef enum TextUnit TextUnit;
typedef enum TextPatternRangeEndpoint TextPatternRangeEndpoint;
typedef enum SynchronizedInputType SynchronizedInputType;
typedef enum ToggleState ToggleState;

typedef struct UiaCondition UiaCondition;
typedef struct UiaPropertyCondition UiaPropertyCondition;
typedef struct UiaAndOrCondition UiaAndOrCondition;
typedef struct UiaNotCondition UiaNotCondition;
typedef struct UiaCacheRequest UiaCacheRequest;
typedef struct UiaFindParams UiaFindParams;
typedef struct UiaEventArgs UiaEventArgs;
typedef struct UiaPropertyChangedEventArgs UiaPropertyChangedEventArgs;
typedef struct UiaStructureChangedEventArgs UiaStructureChangedEventArgs;
typedef struct UiaChangeInfo UiaChangeInfo;
typedef struct UiaPoint UiaPoint;
