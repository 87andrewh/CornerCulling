#include "CullingController.h"
#include "OccludingCuboid.h"
#include "OccludingSphere.h"
#include "EngineUtils.h"
#include <chrono> 

ACullingController::ACullingController()
    : Super()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickGroup = TG_PrePhysics;
}

void ACullingController::BeginPlay()
{
    Super::BeginPlay();
    // Add characters.
    for (ACornerCullingCharacter* Player : TActorRange<ACornerCullingCharacter>(GetWorld()))
    {
        Characters.emplace_back(Player);
        IsAlive.emplace_back(true);
        Teams.emplace_back(Player->Team);
    }
    // Add occluding cuboids.
    for (AOccludingCuboid* C : TActorRange<AOccludingCuboid>(GetWorld()))
    {
        Cuboids.emplace_back(Cuboid(C->Vertices));
    }
    if (Cuboids.size() > 0)
    {
        // Build the cuboid BVH.
        FastBVH::BuildStrategy<float, 1> Builder;
        CuboidBoxConverter Converter;
        CuboidBVH = std::make_unique
            <FastBVH::BVH<float, Cuboid>>
            (Builder(Cuboids, Converter));
        CuboidTraverser = std::make_unique
            <Traverser<float, decltype(Intersector)>>
            (*CuboidBVH.get(), Intersector);
    }
    // Add occluding spheres.
    for (AOccludingSphere* S : TActorRange<AOccludingSphere>(GetWorld()))
    {
        Spheres.emplace_back(Sphere(S->GetActorLocation(), S->Radius));
    }
}

void ACullingController::Tick(float DeltaTime)
{
    TotalTicks++;
    BenchmarkCull();
}

void ACullingController::BenchmarkCull()
{
    auto Start = std::chrono::high_resolution_clock::now();
    Cull();
    auto Stop = std::chrono::high_resolution_clock::now();
    UpdateVisibility();
    int Delta = std::chrono::duration_cast<std::chrono::microseconds>(Stop - Start).count();
    TotalTime += Delta;
    RollingTotalTime += Delta;
    RollingMaxTime = std::max(RollingMaxTime, Delta);
    if ((TotalTicks % RollingWindowLength) == 0)
    {
        RollingAverageTime = RollingTotalTime / RollingWindowLength;
        if (GEngine)
        {
            FVector2D Scale = FVector2D(2.0f, 2.0f);
            FColor Color = FColor::Yellow;
            FString Msg = "Average time to cull (microseconds): "
                + FString::FromInt(int(TotalTime / TotalTicks));
            GEngine->AddOnScreenDebugMessage(1, 2.0f, Color, Msg, true, Scale);
            Msg = "Rolling average time to cull (microseconds): "
                + FString::FromInt(int(RollingAverageTime));
            GEngine->AddOnScreenDebugMessage(2, 2.0f, Color, Msg, true, Scale);
            Msg = "Rolling max time to cull (microseconds): "
                + FString::FromInt(RollingMaxTime);
            GEngine->AddOnScreenDebugMessage(3, 2.0f, Color, Msg, true, Scale);
        }
        RollingTotalTime = 0;
        RollingMaxTime = 0;
    }
}

void ACullingController::Cull()
{
    // TODO:
    //   When running multiple servers per CPU, consider staggering
    //   culling periods to avoid lag spikes.
    if ((TotalTicks % CullingPeriod) == 0)
    {
        UpdateCharacterBounds();
        PopulateBundles();
        CullWithCache();
        CullWithSpheres();
        CullWithCuboids();
    }
}

void ACullingController::UpdateCharacterBounds()
{
    Bounds.clear();
    // This block simulates latency for testing. Remove in production.
    // Note that this simulation differs subtly from the real setting,
    // as a real server defines the exact location of all players
    // that are not controlled by the client that it is culling for.
    // In this simulation, the game displays the current positions of enemies,
    // but the server calculates LOS with delayed positions.
    if (CULLING_SIMULATED_LATENCY > 0)
    {
        // Equivalent to
        if (PastBounds.size() >= 3)
        {
            PastBounds.pop_front();
        }
        std::vector<CharacterBounds> CurrentBounds;
        for (int i = 0; i < Characters.size(); i++)
        {
            if (IsAlive[i])
            {
                CurrentBounds.emplace(
                    CurrentBounds.begin() + i,
                    CharacterBounds(
                        Characters[i]
                        ->GetFirstPersonCameraComponent()
                        ->GetComponentLocation(),
                        Characters[i]->GetActorTransform()));
            }
        }
        PastBounds.emplace_back(CurrentBounds);
        Bounds = PastBounds[0];
    }
    else
    {
        for (int i = 0; i < Characters.size(); i++)
        {
            if (IsAlive[i])
            {
                Bounds.emplace(
                    Bounds.begin() + i,
                    CharacterBounds(
                        Characters[i]->GetFirstPersonCameraComponent()->GetComponentLocation(),
                        Characters[i]->GetActorTransform()));
            }
        }
    }
}

void ACullingController::PopulateBundles()
{
    BundleQueue.clear();
    for (int i = 0; i < Characters.size(); i++)
    {
        if (IsAlive[i])
        {
            // TODO:
            //   Make displacement a function of game physics and state.
            float Latency = GetLatency(i);
            float MaxHorizontalDisplacement = Latency * 350;
            float MaxVerticalDisplacement = Latency * 200;
            for (int j = 0; j < Characters.size(); j++)
            {
                if (VisibilityTimers[i][j] == 0
                    && IsAlive[j]
                    && (Teams[i] != Teams[j]))
                {
                    BundleQueue.emplace_back(
                        Bundle(
                            i,
                            j,
                            GetPossiblePeeks(
                                Bounds[i].CameraLocation,
                                Bounds[j].Center,
                                MaxHorizontalDisplacement,
                                MaxVerticalDisplacement)));
                }
            }
        }
    }
}

// Estimates the latency of the client controlling character i in seconds.
// The estimate should be greater than the expected latency,
// as underestimating latency results in underestimated peeks,
// which could result in popping.
// TODO:
//   Integrate with server latency estimation tools.
float ACullingController::GetLatency(int i)
{
    return float(CULLING_SIMULATED_LATENCY) / SERVER_TICKRATE;
}

std::vector<FVector> ACullingController::GetPossiblePeeks(
    const FVector& PlayerCameraLocation,
    const FVector& EnemyLocation,
    float MaxDeltaHorizontal,
    float MaxDeltaVertical)
{
    std::vector<FVector> Corners;
    FVector PlayerToEnemy =
        (EnemyLocation - PlayerCameraLocation).GetSafeNormal(1e-6);
    // Displacement parallel to the XY plane and perpendicular to PlayerToEnemy.
    FVector Horizontal =
        MaxDeltaHorizontal * FVector(-PlayerToEnemy.Y, PlayerToEnemy.X, 0);
    FVector Vertical = FVector(0, 0, MaxDeltaVertical);
    Corners.emplace_back(PlayerCameraLocation + Horizontal + Vertical);
    Corners.emplace_back(PlayerCameraLocation - Horizontal + Vertical);
    Corners.emplace_back(PlayerCameraLocation - Horizontal - Vertical);
    Corners.emplace_back(PlayerCameraLocation + Horizontal - Vertical);
    return Corners;
}

void ACullingController::CullWithCache()
{
    std::vector<Bundle> Remaining;
    for (Bundle B : BundleQueue)
    {
        bool Blocked = false;
        for (int k = 0; k < CUBOID_CACHE_SIZE; k++)
        {
            if (CuboidCaches[B.PlayerI][B.EnemyI][k] != NULL)
            {
                if (
                    IsBlocking(
                        B.PossiblePeeks,
                        Bounds[B.EnemyI],
                        CuboidCaches[B.PlayerI][B.EnemyI][k]))
                {
                    Blocked = true;
                    CacheTimers[B.PlayerI][B.EnemyI][k] = TotalTicks;
                    break;
                }
            }
        }
        if (!Blocked)
        {
            Remaining.emplace_back(B);
        }
    }
    BundleQueue = Remaining;
}

void ACullingController::CullWithSpheres()
{
    std::vector<Bundle> Remaining;
    for (Bundle B : BundleQueue)
    {
        bool Blocked = false;
        for (Sphere S : Spheres)
        {
            if (
                IsBlocking(
                    B.PossiblePeeks,
                    Bounds[B.EnemyI],
                    S))
            {
                Blocked = true;
                break;
            }
        }
        if (!Blocked)
        {
            Remaining.emplace_back(B);
        }
    }
    BundleQueue = Remaining;
}

void ACullingController::CullWithCuboids()
{
    std::vector<Bundle> Remaining;
    for (Bundle B : BundleQueue)
    {
        const Cuboid* CuboidP = CuboidTraverser.get()->traverse(
            OptSegment(
                Bounds[B.PlayerI].CameraLocation,
                Bounds[B.EnemyI].Center),
            B.PossiblePeeks,
            Bounds[B.EnemyI]);
        if (CuboidP != NULL)
        {
            int MinI = ArgMin(
                CacheTimers[B.PlayerI][B.EnemyI],
                CUBOID_CACHE_SIZE);
            CuboidCaches[B.PlayerI][B.EnemyI][MinI] = CuboidP;
            CacheTimers[B.PlayerI][B.EnemyI][MinI] = TotalTicks;
        }
        else
        {
            Remaining.emplace_back(B);
        }
    }
    BundleQueue = Remaining;
}

// Increments visibility timers of bundles that were not culled,
// and reveals enemies with positive visibility timers.
void ACullingController::UpdateVisibility()
{
    // There are bundles remaining from the culling pipeline.
    for (Bundle B : BundleQueue)
    {
        VisibilityTimers[B.PlayerI][B.EnemyI] = VisibilityTimerMax;
    }
    BundleQueue.clear();
    // Reveal
    for (int i = 0; i < Characters.size(); i++)
    {
        if (IsAlive[i])
        {
            for (int j = 0; j < Characters.size(); j++)
            {
                if (IsAlive[j] && (VisibilityTimers[i][j] > 0))
                {
                    SendLocation(i, j);
                    VisibilityTimers[i][j]--;
                }
            }
        }
    }
}

// Draws a line from character i to j, simulating the sending of a location.
// TODO:
//   This method is currently just a visualization placeholder,
//   so integrate server location-sending API when deploying to a game.
void ACullingController::SendLocation(int i, int j)
{
    if (Teams[i] == 0)
    {
        ConnectVectors(
            GetWorld(),
            Characters[i]->GetActorLocation() + FVector(0, 0, 40),
            Characters[j]->GetActorLocation(),
            false,
            0.02,
            7.0f,
            FColor::Green);
    }
    else if (Teams[i] == 1)
    {
        return;  //  Showing LOS of both teams is a bit cluttered and confusing.
        ConnectVectors(
            GetWorld(),
            Characters[i]->GetActorLocation() + FVector(0, 0, 40),
            Characters[j]->GetActorLocation(),
            false,
            0.02,
            10.0f,
            FColor(225, 0, 0, 1));
    }
}
