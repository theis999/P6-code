using Microsoft.AspNetCore.Mvc;
using Microsoft.AspNetCore.Authentication;
using Microsoft.AspNetCore.Authorization;
using Microsoft.AspNetCore.Mvc.RazorPages;
using System.Net.Http;
using System.Security.Claims;
using System;
using System.Text;
using System.Text.Json;
using System.Text.Json.Nodes;
using System.Diagnostics;
using System.Net.Http.Headers;


namespace newnewWebinterface.Pages;

[Authorize]
public class RegisterModel : PageModel
{
    [BindProperty]
    public string name { get; set; }
    [BindProperty]
    public string ProductID { get; set; }
    public string? AccessToken { get; set; }

    public async Task<IActionResult> OnPostAsync()
    {

        string authentikUserID = User.FindFirstValue("sub");
        string authentikUserName = User.FindFirstValue("name");

        using StringContent jsonContent = new(
         JsonSerializer.Serialize(new
         {
             name = name,
             product_id = ProductID,
              user = new {
                name = authentikUserName,
                authentik_user_id = authentikUserID
        }
             
         }),
        System.Text.Encoding.UTF8,
        "application/json");

        var client = new HttpClient();


        AccessToken = await HttpContext.GetTokenAsync("access_token");
        client.DefaultRequestHeaders.Authorization = new AuthenticationHeaderValue("Bearer", AccessToken);


        using HttpResponseMessage response = await client.PostAsync("https://smakdb.head9x.dk/boards", jsonContent);

        // Debug
        System.Diagnostics.Debug.WriteLine("Token: "+ AccessToken);
        System.Diagnostics.Debug.WriteLine("Response: "+ response);
        System.Diagnostics.Debug.WriteLine("JsonContent: "+ jsonContent);

        return RedirectToPage("./Index");
    }
}
