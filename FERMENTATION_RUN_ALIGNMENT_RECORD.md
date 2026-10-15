# Kaong Wine Thesis: Group Alignment & Dual-Batch Protocol Record
*Document Date: October 16, 2026 (06:50 PHT)*
*Project: Automated Dual-ESP32 Kaong Wine Brewing Platform (CPEN 200B)*
*Collaborators: Dave Patrick Bulaso, John Martin Ejercitado, Gian, Harley*

---

## 1. Executive Summary
Between October 15, 2026 (20:00 PHT) and October 16, 2026 (06:40 PHT), the capstone group debated the scientific and operational methodology for the comparative fermentation trial. The central question was whether the **Traditional Control setup** should include commercial wine yeast (*Saccharomyces cerevisiae*) or follow 100% wild spontaneous fermentation as practiced in Indang and documented by the 2014 Cavite State University (CvSU) thesis.

By analyzing the physical manuscript scanned from the CvSU library, the group confirmed that the 2014 baseline is **100% wild spontaneous fermentation without added yeast**, clearing all confusion caused by scrambled PDF pages and literature citations.

---

## 2. Chronological Thread Breakdown

### Phase 1: The Initial Yeast & Setup Debate (Oct 15, ~20:00 - 22:30 PHT)
* **Gian & John Martin's Initial Viewpoint**: Argued that wine cannot ferment without yeast, proposing to add commercial wine yeast (*Saccharomyces cerevisiae*) to both the automated machine and the traditional control carboy.
* **Harley's 3-Setup Proposal**: Suggested dividing the sap into three batches:
  1. Raw sap without yeast (ambient wild fermentation).
  2. Preheated sap without yeast (scientifically invalid, as pasteurization thermal shock arrests endogenous flora with no inoculant to restart it).
  3. Preheated sap with wine yeast (automated machine protocol).
* **Dave's Clarification**: Rejected the 3-setup split. Explained that dividing limited sap wastes volume, and the approved Chapter 1 manuscript explicitly defines evaluating the automated machine against the **traditional Indang method**, not an artificial sterilized dead control.
* **Testing & Parameters Debate**: Gian questioned how to verify ABV without costly third-party lab testing. Dave established that primary tracking relies on RAPT pill specific gravity telemetry, calibrated hydrometers, and optical refractometers, which are later verified against standard conversion tables and final sensory evaluations.

### Phase 2: The 2014 CvSU Study Citation Confusion (Oct 15, ~22:30 - 23:45 PHT)
* **The Shared Manuscript**: John Martin provided a 17-page photographed scan of the physical thesis from the CvSU library titled *Fermentation Vessels for Sugar Palm Wine Production* (CvSU Indang, Aug 2014 - Feb 2015).
* **The Confusion**: Martin screenshotted a passage from Chapter 4 (Results & Discussion):
  > *"The fermentation of kaongsap was facilitated by the microorganism or fermenting agents like Saccharomyces cerevisaeand Candida,as cited in the study of the fermentation of different palm sap from coconut, palmyra palm and silver date palm, (Chandrasekhar et al, 2012)."*
* **The Misinterpretation**: Martin believed this sentence proved the 2014 study added yeast.
* **The Rebuttal**: Dave demonstrated that this sentence is a literature citation of Chandrasekhar et al. (2012) describing the naturally occurring wild microorganisms indigenous to palm sap, not an added ingredient.

### Phase 3: Scrambled Pages & The "Missing Item 6" Resolution (Oct 16, ~05:45 - 06:40 PHT)
* **Martin's Doubt**: Martin suggested: *"pinucturan ko oang kasi sa library yan... baka may 6 onwards pa... diko sure"*, fearing that the materials list cut off before mentioning yeast.
* **Structural Page Audit**: Deep inspection of all 17 scanned pages revealed that Martin's photos were bound out of order in the original PDF:
  * PDF Page 2 is physical **Page 11** (Methodology opener and Materials 1 through 5).
  * PDF Page 1 is physical **Page 12** (5-step Wine Production procedure).
  * PDF Page 7 is physical **Page 13** (Parameters, Sensory panel with HRM students, and 2-factor CRD design).
  * PDF Page 8 is physical **Page 14** (Results & Discussion opener).
* **Irrefutable Proof of Zero Added Yeast**:
  1. **Page 11 Ends at Item 5**: Materials list terminates cleanly at `5. Casserole` with significant blank white margin at the bottom.
  2. **Page 12 Contains Full Procedure**: Detail steps strictly cover:
     - Step 1 (Collection): Fresh sap transported in 5-gallon slim container (Item 4).
     - Step 2 (Fermentation): Measured 2 liters using 1000-mL graduated cylinder (Item 3) and poured directly into carboys (Item 2). No yeast or chemicals added.
     - Step 3 (Environment): Open vs. closed room.
     - Step 4 (Pasteurization): Heat-arrested in aluminum casserole (Item 5) at 60°C for 10 minutes upon reaching target acidity.
     - Step 5 (Bottling): Transferred into sterilized bottles.
  3. **Consecutive Pagination**: Physical pages run 11 -> 12 -> 13 -> 14 without any missing leaves.

---

## 3. Ground-Truth Dual-Batch Protocol

| Parameter | Setup A: Traditional Control | Setup B: Automated Machine Run |
| :--- | :--- | :--- |
| **Methodology Reference** | CvSU 2014 Study (Indang Traditional Practice) | Approved CEIT CPEN 200B Thesis Protocol |
| **Vessel Type** | Food-grade Glass Carboy | Automated Stainless Steel / Acrylic Reactor |
| **Sap Treatment** | Raw, unheated fresh kaong sap | Controlled pre-heating / pasteurization pass |
| **Fermentation Agent** | 100% Wild Microflora (*Candida*, wild yeasts) | Commercial *Saccharomyces cerevisiae* Inoculation |
| **Environmental Control**| Ambient room temperature | Closed-loop temperature regulation & active monitoring |
| **Telemetry Tracking** | Manual hourly hydrometer / refractometer / pH probe | Dual ESP32 + RAPT Pill continuous automated telemetry |
| **Termination Cutoff** | Manual pasteurization at 60°C (10 mins) at target pH | Automated alarm / cutoff at target pH |

---

## 4. Current Status & Next Actions
* **Sap Location**: Preserved in home freezer (retrieved October 15).
* **Alignment Status**: Awaiting final acknowledgment from Gian and John Martin in Messenger group chat following Dave's summary message.
* **Immediate Action**:
  1. Thaw the frozen kaong sap.
  2. Measure initial baseline parameters: Brix/TSS, pH, temperature, and specific gravity.
  3. Divide volume evenly between Setup A (Carboy) and Setup B (Automated Brewing System).
  4. Initiate telemetry logging via `winebrew-logger.service` and start continuous monitoring.
