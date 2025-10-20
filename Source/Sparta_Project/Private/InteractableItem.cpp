#include "InteractableItem.h"
#include "Components/WidgetComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"

AInteractableItem::AInteractableItem()
{
	PrimaryActorTick.bCanEverTick = true;

	InteractionWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("InteractionWidget"));
	InteractionWidget->SetupAttachment(RootComponent);
	InteractionWidget->SetWidgetSpace(EWidgetSpace::World);
	InteractionWidget->SetDrawSize(FVector2D(300.f, 100.f));

}

void AInteractableItem::OnInteract()
{
	Destroy();
}

void AInteractableItem::BeginPlay()
{
	Super::BeginPlay();
	InteractionWidget->SetVisibility(false);
}

void AInteractableItem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (InteractionWidget)
	{
		APlayerCameraManager* Camera = UGameplayStatics::GetPlayerCameraManager(this, 0);

		if (Camera)
		{
			FVector CamLocation = Camera->GetCameraLocation();
			FVector ToCamera = CamLocation - InteractionWidget->GetComponentLocation();
			ToCamera.Normalize();

			FRotator LookAtRotation = ToCamera.Rotation();
			InteractionWidget->SetWorldRotation(LookAtRotation);
		}
	}

}

void AInteractableItem::OnItemOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSeep, const FHitResult& SweepResult)
{
	InteractionWidget->SetVisibility(true);
}

void AInteractableItem::OnItemEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	InteractionWidget->SetVisibility(false);
}
